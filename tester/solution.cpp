// /*
//  * solution.cpp
//  *
//  * Example solution.
//  * This is (almost) how blocks are actually compressed in TON.
//  * Normally, blocks are stored using vm::std_boc_serialize with mode=31.
//  * Compression algorithm takes a block, converts it to mode=2 (which has less extra information) and compresses it using lz4.
//  */
// #include <iostream>
// #include "td/utils/lz4.h"
// #include "td/utils/buffer.h" 
// #include "td/utils/misc.h"  
// #include "common/util.h"    
// #include "vm/cellslice.h" 

// // #include "td/utils/base64.h"
// #include "vm/boc.h"

// td::BufferSlice compress(td::Slice data) {

//   td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
//   td::BufferSlice serialized = vm::std_boc_serialize(root, 2).move_as_ok();

//   return td::lz4_compress(serialized);
// }

// td::BufferSlice decompress(td::Slice data) {
//   td::BufferSlice serialized = td::lz4_decompress(data, 2 << 20).move_as_ok();

//   auto root = vm::std_boc_deserialize(serialized).move_as_ok();
//   return vm::std_boc_serialize(root, 31).move_as_ok();
// }

// int main() {
//   std::string mode;
//   std::cin >> mode;
//   CHECK(mode == "compress" || mode == "decompress");

//   std::string base64_data;
//   std::cin >> base64_data;
//   CHECK(!base64_data.empty());

//   td::BufferSlice data(td::base64_decode(base64_data));


//   if (mode == "compress") {
//     data = compress(data);
//   } else {
//     data = decompress(data);
//   }

//   std::cout << td::str_base64_encode(data) << std::endl;
// }
#include <iostream>
#include <vector>
#include "td/utils/lz4.h"
#include "td/utils/buffer.h" 
#include "td/utils/misc.h"  
#include "common/util.h"    
#include "vm/cellslice.h" 

// #include "td/utils/base64.h"
#include "vm/boc.h"

// Substitution Cipher Mapping (example)
std::vector<uint8_t> substitution_map(256);
std::vector<uint8_t> reverse_map(256);

// Initialize substitution and reverse maps
void initialize_cipher() {
  for (int i = 0; i < 256; i++) {
    substitution_map[i] = (i + 47) % 256;  // Example: Rotate by 47
    reverse_map[substitution_map[i]] = i; // Reverse mapping
  }
}

// Apply substitution cipher
void apply_cipher(td::MutableSlice data) {
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = substitution_map[data[i]];
  }
}

// Reverse substitution cipher
void reverse_cipher(td::MutableSlice data) {
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = reverse_map[data[i]];
  }
}

// Compression with substitution cipher
td::BufferSlice compress(td::Slice data) {
  td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
  td::BufferSlice serialized = vm::std_boc_serialize(root, 2).move_as_ok();

  // Apply substitution cipher
  apply_cipher(serialized.as_slice());

  return td::lz4_compress(serialized);
}

// Decompression with substitution cipher
td::BufferSlice decompress(td::Slice data) {
  td::BufferSlice serialized = td::lz4_decompress(data, 2 << 20).move_as_ok();

  // Reverse substitution cipher
  reverse_cipher(serialized.as_slice());

  auto root = vm::std_boc_deserialize(serialized).move_as_ok();
  return vm::std_boc_serialize(root, 31).move_as_ok();
}

int main() {
  initialize_cipher();

  std::string mode;
  std::cin >> mode;
  CHECK(mode == "compress" || mode == "decompress");

  std::string base64_data;
  std::cin >> base64_data;
  CHECK(!base64_data.empty());

  td::BufferSlice data(td::base64_decode(base64_data));

  if (mode == "compress") {
    data = compress(data);
  } else {
    data = decompress(data);
  }

  std::cout << td::str_base64_encode(data) << std::endl;
}
