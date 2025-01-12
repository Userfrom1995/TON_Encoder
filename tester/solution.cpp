
#include "td/utils/buffer.h" 
#include "td/utils/lz4.h" // TON's LZ4 library
#include "vm/boc.h" // For TON serialization/deserialization
#include "common/util.h" 
#include "vm/cellslice.h" 
#include "td/utils/misc.h"
#include "td/utils/Gzip.h"
#include<fstream>
#include<iostream>

// Compress function using gzip
// Compress function using gzip
// Compress function using gzip
td::BufferSlice compress(td::Slice data) {
    try {
        // Deserialize the data
        td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
        td::BufferSlice serialized = vm::std_boc_serialize(root, 0).move_as_ok();
        
        // Define the chunk size for splitting
        const std::size_t chunk_size = 64 * 1024; // 64 KB for example
        
        // Initialize BufferBuilder directly
        td::BufferBuilder final_compressed_builder;
        
        // Split the data into smaller chunks, compress, and append directly
        for (std::size_t i = 0; i < serialized.size(); i += chunk_size) {
            // Create a slice for the current chunk
            td::Slice chunk(serialized.data() + i, std::min(chunk_size, serialized.size() - i));
            
            // Compress the chunk using LZ4
            auto compressed_chunk = td::lz4_compress(chunk);
            
            // Append the compressed chunk to BufferBuilder using move semantics
            final_compressed_builder.append(std::move(compressed_chunk));
        }

        // Extract the final compressed BufferSlice
        td::BufferSlice final_compressed = final_compressed_builder.extract();
        
        // Optionally apply gzip compression for further reduction
        double max_compression_ratio = 100.0; // Adjust as needed
        auto final_compressed_gzipped = td::gzencode(final_compressed, max_compression_ratio);

        if (final_compressed_gzipped.empty()) {
            throw std::runtime_error("Gzip compression returned empty data.");
        }

        return final_compressed_gzipped;
    } catch (const std::exception& e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        return {};
    }
}
// Decompress function using gzip
td::BufferSlice decompress(td::Slice data) {
    try {
        auto decompressed = td::gzdecode(data);
      
        td::BufferSlice serialized = td::lz4_decompress(decompressed, 2 << 20).move_as_ok();
       auto root = vm::std_boc_deserialize(serialized).move_as_ok();
        auto data= vm::std_boc_serialize(root, 31).move_as_ok();
        if (decompressed.empty()) {
            throw std::runtime_error("Gzip decompression returned empty data.");
        }
        return data;
    } catch (const std::exception& e) {
        std::cerr << "Decompression failed: " << e.what() << std::endl;
        return {};
    }
}

int main() {
    try {
        //File paths
        const std::string input_file = "input.txt";
        const std::string output_file = "output.txt";

        //Read input file
        std::ifstream infile(input_file);
        if (!infile.is_open()) {
            throw std::runtime_error("Could not open input file!");
        }

       // Read mode from first line
        std::string mode;
        infile >> mode;
        if (mode != "compress" && mode != "decompress") {
            throw std::runtime_error("Invalid mode. Use 'compress' or 'decompress'.");
        }
        // std::cin>>mode;

        // Read base64 data from second line
        std::string base64_data;
        // std::cin>>base64_data;
        infile >> base64_data;
        if (base64_data.empty()) {
            throw std::runtime_error("Empty data in input file!");
        }
        infile.close();

        // // Decode Base64 input
        td::BufferSlice data = td::base64_decode(base64_data);
        std::cout << "Input size: " << data.size() << " bytes" << std::endl;

        // Process data based on the mode
        if (mode == "compress") {
            data = compress(data);
            std::cout << "Compressed size: " << data.size() << " bytes" << std::endl;
        } else {
            data = decompress(data);
            // std::cout << "Decompressed size: " << data.size() << " bytes" << std::endl;
        }

        //Encode the result to Base64 and write to output file
        std::ofstream outfile(output_file);
        if (!outfile.is_open()) {
            throw std::runtime_error("Could not open output file!");
        }

        std::string output = td::str_base64_encode(data);
        outfile << output << std::endl;
        outfile.close();
        // std::cout << output << std::endl;

        std::cout << "Operation completed. Output written to " << output_file << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
