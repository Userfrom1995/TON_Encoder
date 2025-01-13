#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "td/utils/lz4.h"          // TON's LZ4 library
#include "vm/boc.h"               // For TON serialization/deserialization
#include "td/utils/base64.h"      // For Base64 encoding/decoding

// Convert binary data (byte array) to an ASCII string
std::string binary_to_ascii(const td::BufferSlice& binary_data) {
    return std::string(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
}

// Convert an ASCII string to binary (byte array)
std::vector<unsigned char> ascii_to_binary(const std::string& ascii_str) {
    return std::vector<unsigned char>(ascii_str.begin(), ascii_str.end());
}

// Compress the block using TON's LZ4 and serialization
td::BufferSlice compress_block(td::Slice data) {
    // Deserialize the TON block
    auto root = vm::std_boc_deserialize(data).move_as_ok();

    // Serialize the block using compact mode (mode=2)
    td::BufferSlice serialized = vm::std_boc_serialize(root, 2).move_as_ok();

    // Compress the serialized block using TON's LZ4
    td::BufferSlice compressed = td::lz4_compress(serialized);

    return compressed;
}

// Decompress the block and restore its original format
td::BufferSlice decompress_block(td::Slice data) {
    // Decompress the input data using TON's LZ4
    td::BufferSlice decompressed = td::lz4_decompress(data, 2 << 20).move_as_ok();

    // Deserialize the decompressed TON block
    auto root = vm::std_boc_deserialize(decompressed).move_as_ok();

    // Serialize the block back to the original mode (mode=31)
    return vm::std_boc_serialize(root, 31).move_as_ok();
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
        td::BufferSlice binary_data(td::base64_decode(base64_data).move_as_ok());

        // Step 2: Convert the binary data to an ASCII string
        std::string ascii_str = binary_to_ascii(binary_data);

        // Step 3: If in "compress" mode, process the data
        if (mode == "compress") {
            // Convert ASCII string to binary for processing
            std::vector<unsigned char> binary_input = ascii_to_binary(ascii_str);

            // Compress the binary data (after processing)
            td::BufferSlice compressed_data = compress_block(td::Slice(binary_input.data(), binary_input.size()));

            // Step 4: Encode the compressed result back to Base64
            std::cout << td::base64_encode(compressed_data) << std::endl;

        } else if (mode == "decompress") {
            // Decompress the data
            td::BufferSlice decompressed_data = decompress_block(binary_data);

            // Step 5: Convert decompressed binary back to ASCII
            std::string decompressed_ascii = binary_to_ascii(decompressed_data);

            // Step 6: Encode the decompressed result back to Base64
            std::cout << td::base64_encode(td::BufferSlice(decompressed_ascii)) << std::endl;
        }

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
