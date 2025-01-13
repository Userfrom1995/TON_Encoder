
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
std::string binary_to_ascii(const td::BufferSlice& binary_data) {
    return std::string(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
}

// Convert an ASCII string to binary (byte array)
std::vector<unsigned char> ascii_to_binary(const std::string& ascii_str) {
    return std::vector<unsigned char>(ascii_str.begin(), ascii_str.end());
}

td::BufferSlice compress(td::Slice data) {
    try {
        // Deserialize the data
        td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
        td::BufferSlice serialized = vm::std_boc_serialize(root, 2).move_as_ok();
        
        // Define the chunk size for splitting
        const std::size_t chunk_size =  1024*1024; // 64 KB for example
        
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
        double max_compression_ratio = 1.0; // Adjust as needed
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
      
        td::BufferSlice serialized = td::lz4_decompress(decompressed, 1 << 20).move_as_ok();
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
        std::string mode;
        std::cin >> mode;

        if (mode != "compress" && mode != "decompress") {
            std::cerr << "Invalid mode. Use 'compress' or 'decompress'." << std::endl;
            return 1;
        }

        std::string base64_data;
        std::cin >> base64_data;

        if (base64_data.empty()) {
            std::cerr << "Input data is empty." << std::endl;
            return 1;
        }

        // Step 1: Decode Base64 to binary data
        td::BufferSlice binary_data(td::base64_decode(base64_data));

        // Step 2: Convert the binary data to an ASCII string
        std::string ascii_str = binary_to_ascii(binary_data);

        // Step 3: If in "compress" mode, process the data
        if (mode == "compress") {
            // Convert ASCII string to binary for processing
            std::vector<unsigned char> binary_input = ascii_to_binary(ascii_str);

            // Compress the binary data (after processing)
            td::BufferSlice compressed_data = compress(td::Slice(binary_input.data(), binary_input.size()));

            // Step 4: Encode the compressed result back to Base64
            std::cout << td::str_base64_encode(compressed_data) << std::endl;

        } else if (mode == "decompress") {
            // Decompress the data
            td::BufferSlice decompressed_data = decompress(binary_data);

            // Step 5: Convert decompressed binary back to ASCII
            std::string decompressed_ascii = binary_to_ascii(decompressed_data);

            // Step 6: Encode the decompressed result back to Base64
            std::cout << td::str_base64_encode(td::BufferSlice(decompressed_ascii)) << std::endl;
        }

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
