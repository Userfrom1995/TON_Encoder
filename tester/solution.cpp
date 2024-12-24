#include <iostream>
#include "td/utils/lz4.h"
#include "td/utils/base64.h"
#include "vm/boc.h"

// Preprocessing: Flatten cells to a minimal representation
td::BufferSlice preprocess(td::Slice data) {
    auto root = vm::std_boc_deserialize(data).move_as_ok();
    return vm::std_boc_serialize(root, 1).move_as_ok(); // Use mode=1 for minimal metadata
}

// Compression: Apply preprocessing and LZ4 compression
td::BufferSlice compress(td::Slice data) {
    td::BufferSlice preprocessed = preprocess(data);
    return td::lz4_compress(preprocessed); // Compress preprocessed data
}

// Decompression: Reverse LZ4 and preprocessing
td::BufferSlice decompress(td::Slice data) {
    td::BufferSlice decompressed = td::lz4_decompress(data, 2 << 20).move_as_ok();
    auto root = vm::std_boc_deserialize(decompressed).move_as_ok();
    return vm::std_boc_serialize(root, 31).move_as_ok(); // Restore original metadata
}

int main() {
    std::string mode;
    std::cin >> mode;
    if (mode != "compress" && mode != "decompress") {
        std::cerr << "Invalid mode" << std::endl;
        return 1;
    }

    std::string base64_data;
    std::cin >> base64_data;
    if (base64_data.empty()) {
        std::cerr << "Empty input" << std::endl;
        return 1;
    }

    td::BufferSlice data(td::base64_decode(base64_data).move_as_ok());

    if (mode == "compress") {
        data = compress(data);
    } else {
        data = decompress(data);
    }

    std::cout << td::base64_encode(data) << std::endl;
    return 0;
}
