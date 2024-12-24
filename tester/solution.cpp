#include <iostream>
#include "td/utils/lz4.h"
#include "td/utils/base64.h"
#include "vm/boc.h"

// Preprocessing: Minimize data size
td::BufferSlice preprocess(td::Slice data) {
    auto root = vm::std_boc_deserialize(data).move_as_ok();
    // Use minimal serialization for better compression
    return vm::std_boc_serialize(root, 0).move_as_ok();
}

// Optimized compression
td::BufferSlice compress(td::Slice data) {
    if (data.empty()) {
        return td::BufferSlice();
    }
    // Preprocess with minimal representation
    td::BufferSlice preprocessed = preprocess(data);
    // Compress with standard LZ4
    return td::lz4_compress(preprocessed);
}

// Optimized decompression
td::BufferSlice decompress(td::Slice data) {
    if (data.empty()) {
        return td::BufferSlice();
    }
    const size_t max_size = 2 << 20; // 2MB safety limit
    td::BufferSlice decompressed = td::lz4_decompress(data, max_size).move_as_ok();
    auto root = vm::std_boc_deserialize(decompressed).move_as_ok();
    return vm::std_boc_serialize(root, 31).move_as_ok();
}

// ...rest of main() remains unchanged...
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
