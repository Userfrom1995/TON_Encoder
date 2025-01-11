#include "td/utils/buffer.h"
#include "td/utils/lz4.h"
#include "vm/boc.h"
#include "common/util.h"
#include "vm/cellslice.h"
#include "vm/dict.h"
#include "td/utils/misc.h"
#include "td/utils/Gzip.h"
#include "td/utils/Storer.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

// Dictionary-based pattern storage for improved compression
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
        vm::CellSlice cs{root};
        
        td::BufferBuilder result;
        
        while (!cs.empty()) {
            bool is_pattern = cs.fetch_long(1) == 1;
            
            if (is_pattern) {
                // Read pattern reference
                int pattern_id = cs.fetch_long(16);
                auto pattern_entry = dict.lookup(td::BitArray<16>(pattern_id));
                
                if (!pattern_entry.is_null()) {
                    vm::CellSlice pattern_cs{pattern_entry};
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

// Enhanced compression function using dictionary-based compression
td::BufferSlice compress(td::Slice data) {
    try {
        // First, apply dictionary-based compression
        DictionaryCompressor dict_compressor;
        td::BufferSlice dict_compressed = dict_compressor.compress_with_dict(data);
        
        // Then apply LZ4 compression
        td::BufferSlice lz4_compressed = td::lz4_compress(dict_compressed);
        
        // Finally, apply gzip compression
        td::BufferSlice gzip_compressed = td::gzencode(lz4_compressed, 100.0);
        if (gzip_compressed.empty()) {
            throw std::runtime_error("Gzip compression failed.");
        }

        return gzip_compressed;
    } catch (const std::exception &e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        return {};
    }
}

// Enhanced decompression function
td::BufferSlice decompress(td::Slice data) {
    try {
        // First, decode gzip
        td::BufferSlice gzip_decompressed = td::gzdecode(data);
        
        // Then decompress LZ4
        td::BufferSlice lz4_decompressed = td::lz4_decompress(gzip_decompressed, 2 << 20).move_as_ok();
        
        // Finally, apply dictionary-based decompression
        DictionaryCompressor dict_compressor;
        return dict_compressor.decompress_with_dict(lz4_decompressed);
    } catch (const std::exception &e) {
        std::cerr << "Decompression failed: " << e.what() << std::endl;
        return {};
    }
}