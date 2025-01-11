#include "td/utils/buffer.h"
#include "td/utils/lz4.h" // TON's LZ4 library
#include "vm/boc.h"       // For TON serialization/deserialization
#include "common/util.h"
#include "vm/cellslice.h"
#include "vm/dict.h"
#include "td/utils/misc.h"
#include "td/utils/Gzip.h"
#include "td/utils/Storer.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

class DictionaryCompressor {
    private:
        vm::Dictionary dict{16}; // 16-bit keys for pattern indexing
        unsigned next_pattern_id{0};
        const size_t min_pattern_size{4}; // Minimum pattern size to consider
        const size_t max_pattern_size{64}; // Maximum pattern size to store
    
        // Store pattern in dictionary and return its ID
        int store_pattern(td::Slice pattern) {
            if (pattern.size() < min_pattern_size || pattern.size() > max_pattern_size) {
                return -1;
            }
    
            vm::CellBuilder cb;
            cb.store_bytes(pattern.data(), pattern.size());
            cb.store_long(pattern.size(), 8); // Store pattern length
            
            dict.set_builder(td::BitArray<16>(next_pattern_id), cb);
            return next_pattern_id++;
        }
    
    public:
        // Compress data using dictionary-based pattern matching
        td::BufferSlice compress_with_dict(td::Slice data) {
            vm::CellBuilder result_cb;
            size_t pos = 0;
    
            // First pass: build dictionary of repeated patterns
            while (pos + min_pattern_size <= data.size()) {
                for (size_t len = max_pattern_size; len >= min_pattern_size; len--) {
                    if (pos + len > data.size()) continue;
                    
                    td::Slice pattern = td::Slice(data.data() + pos, len);
                    store_pattern(pattern);
                }
                pos++;
            }
    
            // Second pass: encode data using dictionary references
            pos = 0;
            while (pos < data.size()) {
                bool pattern_found = false;
                
                // Try to find longest matching pattern
                for (size_t len = max_pattern_size; len >= min_pattern_size; len--) {
                    if (pos + len > data.size()) continue;
                    
                    td::Slice current = td::Slice(data.data() + pos, len);
                    
                    // Look up pattern in dictionary
                    for (int i = 0; i < next_pattern_id; i++) {
                        auto dict_entry = dict.lookup(td::BitArray<16>(i));
                        if (!dict_entry.is_null()) {
                            vm::CellSlice cs{dict_entry};
                            size_t pattern_len = cs.fetch_ulong(8);
                            if (pattern_len == len) {
                                // Pattern match found - store reference
                                result_cb.store_long(1, 1); // Flag for pattern reference
                                result_cb.store_long(i, 16); // Pattern ID
                                pos += len;
                                pattern_found = true;
                                break;
                            }
                        }
                    }
                    if (pattern_found) break;
                }
                
                if (!pattern_found) {
                    // Store literal byte
                    result_cb.store_long(0, 1); // Flag for literal
                    result_cb.store_long(data[pos], 8);
                    pos++;
                }
            }
    
            // Serialize the compressed data
            auto cell = result_cb.finalize();
            return vm::std_boc_serialize(cell, 31).move_as_ok();
        }
    
        // Decompress data using stored dictionary
        td::BufferSlice decompress_with_dict(td::Slice compressed_data) {
            auto root = vm::std_boc_deserialize(compressed_data).move_as_ok();
            vm::CellSlice cs(vm::NoVm(), root);
            td::BufferBuilder result;
            
            while (!cs.empty()) {
                bool is_pattern = cs.fetch_long(1) == 1;
                
                if (is_pattern) {
                    // Read pattern reference
                    int pattern_id = cs.fetch_long(16);
                    auto pattern_entry = dict.lookup(td::BitArray<16>(pattern_id));
                    
                    if (!pattern_entry.is_null()) {
                        vm::CellSlice pattern_cs(vm::NoVm(), pattern_entry);
                        size_t pattern_len = pattern_cs.fetch_ulong(8);
                        
                        // Extract and append pattern data
                        unsigned char buffer[max_pattern_size];
                        pattern_cs.fetch_bytes(buffer, pattern_len);
                        result.append(td::Slice(buffer, pattern_len));
                    }
                } else {
                    // Read literal byte
                    unsigned char byte = cs.fetch_long(8);
                    result.append(td::Slice(&byte, 1));
                }
            }
            
            return result.as_buffer_slice();
        }
    };
    

// Compress function using SliceStorer and ConcatStorer
td::BufferSlice compress(td::Slice data) {
    try {
        // Deserialize the input slice into a Cell
        td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();

        // // Serialize the Cell into a buffer
        td::BufferSlice serialized = vm::std_boc_serialize(root, 0).move_as_ok();

        // // Efficiently handle the serialized slice with SliceStorer
        // td::SliceStorer storer(serialized);
        // td::BufferSlice storer_buffer(storer.size());
        // storer.store(storer_buffer.as_slice().ubegin());

          // First, apply dictionary-based compression
        DictionaryCompressor dict_compressor;
        td::BufferSlice dict_compressed = dict_compressor.compress_with_dict(serialized);
        
        // Apply LZ4 compression
        td::BufferSlice lz4_compressed = td::lz4_compress(dict_compressed);

        // // Add metadata (e.g., version or additional info) using ConcatStorer
        // std::string header = "COMP1";  // Example header
        // td::Slice header_slice(header);
        // td::SliceStorer header_storer(header_slice);
        // td::ConcatStorer combined_storer(header_storer, create_storer(lz4_compressed));

        // // Allocate buffer for combined data
        // td::BufferSlice combined_buffer(combined_storer.size());
        // combined_storer.store(combined_buffer.as_slice().ubegin());

        // Compress combined data with gzip for further size reduction
        td::BufferSlice gzip_compressed =
        td::gzencode(lz4_compressed, 100.0);
        if (gzip_compressed.empty()) {
            throw std::runtime_error("Gzip compression failed.");
        }

        return gzip_compressed;
    } catch (const std::exception &e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        return {};
    }
}

td::BufferSlice decompress(td::Slice data) {
    try {
        // Decode gzip-compressed data
        td::BufferSlice gzip_decompressed = td::gzdecode(data);

        // Extract the header and payload using SliceStorer
        // const size_t header_size = 5;  // Example header size
        //         // Extract the header and payload manually
        //         td::Slice gzip_decompressed_slice = gzip_decompressed.as_slice(); // Convert BufferSlice to Slice
        //         td::Slice header = td::Slice(gzip_decompressed_slice.begin(), header_size);
        //         td::Slice payload = td::Slice(gzip_decompressed_slice.begin() + header_size,
        //                                       gzip_decompressed_slice.size() - header_size);
        
        // if (header != "COMP1") {
        //     throw std::runtime_error("Invalid header detected.");
        // }

        // Decompress the payload using LZ4
        td::BufferSlice lz4_decompressed = td::lz4_decompress(data, 2 << 20).move_as_ok();

        // Deserialize the decompressed data back into a Cell
        // td::Ref<vm::Cell> root = vm::std_boc_deserialize(lz4_decompressed).move_as_ok();

        // // Re-serialize the Cell into a BufferSlice
        // td::BufferSlice serialized = vm::std_boc_serialize(root, 31).move_as_ok();

        return lz4_decompressed;
    } catch (const std::exception &e) {
        std::cerr << "Decompression failed: " << e.what() << std::endl;
        return {};
    }
}

int main() {
    try {
        // File paths
        const std::string input_file = "input.txt";
        const std::string output_file = "output.txt";

        // Read input file
        std::ifstream infile(input_file);
        if (!infile.is_open()) {
            throw std::runtime_error("Could not open input file!");
        }

        // Read mode from the first line
        std::string mode;
        infile >> mode;
        if (mode != "compress" && mode != "decompress") {
            throw std::runtime_error("Invalid mode. Use 'compress' or 'decompress'.");
        }

        // Read base64 data from the second line
        std::string base64_data;
        infile >> base64_data;
        if (base64_data.empty()) {
            throw std::runtime_error("Empty data in input file!");
        }
        infile.close();

        // Decode Base64 input
        td::BufferSlice data = td::base64_decode(base64_data);
        std::cout << "Input size: " << data.size() << " bytes" << std::endl;

        // Process data based on the mode
        if (mode == "compress") {
            data = compress(data);
            std::cout << "Compressed size: " << data.size() << " bytes" << std::endl;
        } else {
            data = decompress(data);
            std::cout << "Decompressed size: " << data.size() << " bytes" << std::endl;
        }

        // Encode the result to Base64 and write to the output file
        std::ofstream outfile(output_file);
        if (!outfile.is_open()) {
            throw std::runtime_error("Could not open output file!");
        }

        std::string output = td::str_base64_encode(data);
        outfile << output << std::endl;
        outfile.close();

        std::cout << "Operation completed. Output written to " << output_file << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
