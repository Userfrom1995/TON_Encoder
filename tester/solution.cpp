#include <iostream>
#include <string>
#include "td/utils/lz4.h"
#include "td/utils/base64.h"
#include "vm/boc.h"

// Compress the block using serialization and LZ4
td::BufferSlice compress_block(td::Slice data) {
    // Deserialize the input TON block
    auto root = vm::std_boc_deserialize(data).move_as_ok();

    // Serialize the block using a compact format (mode=2)
    td::BufferSlice serialized = vm::std_boc_serialize(root, 3).move_as_ok();

    // Compress the serialized block using LZ4
    return td::lz4_compress(serialized);
}

// Decompress the block and restore it to the original format
td::BufferSlice decompress_block(td::Slice data) {
    // Decompress the input data using LZ4
    td::BufferSlice decompressed = td::lz4_decompress(data, 2 << 20).move_as_ok();

    // Deserialize the block
    auto root = vm::std_boc_deserialize(decompressed).move_as_ok();

    // Serialize the block back to the original mode (mode=31)
    return vm::std_boc_serialize(root, 31).move_as_ok();
}

int main() {
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

    // Decode Base64 input
    td::BufferSlice data(td::base64_decode(base64_data).move_as_ok());

    if (mode == "compress") {
        // Compress the data
        data = compress_block(data);
    } else if (mode == "decompress") {
        // Decompress the data
        data = decompress_block(data);
    }

    // Encode result to Base64 and output
    std::cout << td::base64_encode(data) << std::endl;
    return 0;
}