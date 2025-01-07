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
// #include <iostream>
// #include <vector>
// #include "td/utils/lz4.h"
// #include "td/utils/buffer.h" 
// #include "td/utils/misc.h"  
// #include "common/util.h"    
// #include "vm/cellslice.h" 

// // #include "td/utils/base64.h"
// #include "vm/boc.h"

// // Substitution Cipher Mapping (example)
// std::vector<uint8_t> substitution_map(256);
// std::vector<uint8_t> reverse_map(256);

// // Initialize substitution and reverse maps
// void initialize_cipher() {
//   for (int i = 0; i < 256; i++) {
//     substitution_map[i] = (i + 47) % 256;  // Example: Rotate by 47
//     reverse_map[substitution_map[i]] = i; // Reverse mapping
//   }
// }

// // Apply substitution cipher
// void apply_cipher(td::MutableSlice data) {
//   for (size_t i = 0; i < data.size(); i++) {
//     data[i] = substitution_map[data[i]];
//   }
// }

// // Reverse substitution cipher
// void reverse_cipher(td::MutableSlice data) {
//   for (size_t i = 0; i < data.size(); i++) {
//     data[i] = reverse_map[data[i]];
//   }
// }

// // Compression with substitution cipher
// td::BufferSlice compress(td::Slice data) {
//   td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
//   td::BufferSlice serialized = vm::std_boc_serialize(root, 2).move_as_ok();

//   // Apply substitution cipher
//   apply_cipher(serialized.as_slice());

//   return td::lz4_compress(serialized);
// }

// // Decompression with substitution cipher
// td::BufferSlice decompress(td::Slice data) {
//   td::BufferSlice serialized = td::lz4_decompress(data, 2 << 20).move_as_ok();

//   // Reverse substitution cipher
//   reverse_cipher(serialized.as_slice());

//   auto root = vm::std_boc_deserialize(serialized).move_as_ok();
//   return vm::std_boc_serialize(root, 31).move_as_ok();
// }

// int main() {
//   initialize_cipher();

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
#include <bits/stdc++.h>
#include <string>
#include <queue>
#include <unordered_map>
#include <fstream>
#include "td/utils/buffer.h" 
#include "td/utils/lz4.h" // TON's LZ4 library
#include "vm/boc.h" // For TON serialization/deserialization
#include "common/util.h" 
#include "vm/cellslice.h" 
#include "td/utils/misc.h"
#include "td/utils/Gzip.h"
// #include <zlib.h>  // For Base64 encoding/decoding

// Huffman Tree Node structure
// struct HNode {
//     char c;
//     size_t freq;
//     HNode *left, *right;
//     HNode(char c, size_t f) : c(c), freq(f), left(nullptr), right(nullptr) {}
// };

// // Helper functions for Huffman coding
// std::string serialize_huffman_tree(HNode* root) {
//     if (!root) return "N";
//     if (!root->left && !root->right) 
//         return "L" + std::string(1, root->c);
//     return "B" + serialize_huffman_tree(root->left) + serialize_huffman_tree(root->right);
// }

// HNode* deserialize_huffman_tree(const std::string& data, size_t& pos) {
//     if (pos >= data.length()) return nullptr;
//     char type = data[pos++];
//     if (type == 'N') return nullptr;
//     if (type == 'L') {
//         return new HNode(data[pos++], 0);
//     }
//     HNode* node = new HNode('\0', 0);
//     node->left = deserialize_huffman_tree(data, pos);
//     node->right = deserialize_huffman_tree(data, pos);
//     return node;
// }

// std::string haufmann(const std::string& input) {
//     // Count frequencies
//     std::unordered_map<char, size_t> freq;
//     for (char c : input) freq[c]++;
    
//     // Create priority queue
//     auto comp = [](HNode* a, HNode* b) { return a->freq > b->freq; };
//     std::priority_queue<HNode*, std::vector<HNode*>, decltype(comp)> pq(comp);
//     for (const auto& p : freq) {
//         pq.push(new HNode(p.first, p.second));
//     }
    
//     // Build Huffman tree
//     while (pq.size() > 1) {
//         HNode* left = pq.top(); pq.pop();
//         HNode* right = pq.top(); pq.pop();
//         HNode* parent = new HNode('\0', left->freq + right->freq);
//         parent->left = left;
//         parent->right = right;
//         pq.push(parent);
//     }
//     HNode* root = pq.top();
    
//     // Generate codes
//     std::unordered_map<char, std::string> codes;
//     std::function<void(HNode*, std::string)> generate_codes = [&](HNode* node, std::string code) {
//         if (!node) return;
//         if (!node->left && !node->right) codes[node->c] = code;
//         generate_codes(node->left, code + "0");
//         generate_codes(node->right, code + "1");
//     };
//     generate_codes(root, "");
    
//     // Encode data
//     std::string encoded;
//     for (char c : input) encoded += codes[c];
    
//     // Add padding info
//     char padding = (8 - (encoded.length() % 8)) % 8;
//     encoded += std::string(padding, '0');
    
//     // Convert to bytes
//     std::string result;
//     for (size_t i = 0; i < encoded.length(); i += 8) {
//         char byte = 0;
//         for (size_t j = 0; j < 8; j++)
//             byte = (byte << 1) | (encoded[i + j] == '1');
//         result += byte;
//     }
    
//     // Serialize tree for lossless decompression
//     std::string tree = serialize_huffman_tree(root);
    
//     // Format: tree_size(4bytes) + padding(1byte) + tree + compressed_data
//     std::string size_bytes(4, 0);
//     for (int i = 0; i < 4; i++)
//         size_bytes[i] = (tree.length() >> (i * 8)) & 0xFF;
    
//     return size_bytes + char(padding) + tree + result;
// }

// std::string dehaufmann(const std::string& input) {
//     if (input.length() < 5) return "";
    
//     // Extract tree size and padding
//     size_t tree_size = 0;
//     for (int i = 0; i < 4; i++)
//         tree_size |= (static_cast<unsigned char>(input[i]) << (i * 8));
//     char padding = input[4];
    
//     // Extract and deserialize tree
//     std::string tree_str = input.substr(5, tree_size);
//     size_t pos = 0;
//     HNode* root = deserialize_huffman_tree(tree_str, pos);
    
//     // Convert compressed data to bits
//     std::string bits;
//     for (size_t i = 5 + tree_size; i < input.length(); i++) {
//         char byte = input[i];
//         for (int j = 7; j >= 0; j--)
//             bits += ((byte >> j) & 1) ? '1' : '0';
//     }
    
//     // Remove padding
//     bits = bits.substr(0, bits.length() - padding);
    
//     // Decode using tree
//     std::string result;
//     HNode* curr = root;
//     for (char bit : bits) {
//         curr = (bit == '0') ? curr->left : curr->right;
//         if (!curr->left && !curr->right) {
//             result += curr->c;
//             curr = root;
//         }
//     }
    
//     return result;
// }

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

// Helper function to validate BOC
// td::Ref<vm::Cell> safe_deserialize_boc(const std::string &base64_data) {
//     td::BufferSlice raw = td::base64_decode(base64_data);
//     auto result = vm::std_boc_deserialize(raw.as_slice());
//     if (result.is_error()) {
//         std::string msg = result.error().public_message();
//         if (msg.find("bag of cells is expected to have exactly one root") != std::string::npos) {
//             throw std::runtime_error("Multiple BOC roots detected. Please provide a single-root BOC.");
//         }
//         throw std::runtime_error("BOC deserialization failed: " + msg);
//     }
//     return result.move_as_ok();
// }
std::string haufmann(const std::string& input) {
    if (input.empty()) return "";

    // 1. Calculate character frequencies
    std::unordered_map<char, int> freq;
    for (char c : input) {
        freq[c]++;
    }

    // 2. Create header with character encoding info
    std::string header;
    // Store number of unique characters
    uint16_t dictSize = freq.size();
    header.append(reinterpret_cast<char*>(&dictSize), sizeof(dictSize));

    // Store character-frequency pairs
    for (const auto& pair : freq) {
        header += pair.first;
        header.append(reinterpret_cast<const char*>(&pair.second), sizeof(int));
    }

    // 3. Store original string length for verification
    uint32_t originalLength = input.length();
    header.append(reinterpret_cast<char*>(&originalLength), sizeof(originalLength));

    // 4. Simple character substitution for testing
    std::unordered_map<char, char> encoding;
    char substitution = 33; // Start with '!'
    for (const auto& pair : freq) {
        encoding[pair.first] = substitution++;
    }

    // 5. Encode the actual data
    std::string encodedData;
    for (char c : input) {
        encodedData += encoding[c];
    }

    return header + encodedData;
}

std::string dehaufmann(const std::string& input) {
    if (input.empty()) return "";

    size_t pos = 0;

    // 1. Read dictionary size
    if (pos + sizeof(uint16_t) > input.size()) {
        throw std::runtime_error("Invalid compressed data: too short for dictionary size");
    }
    uint16_t dictSize;
    std::memcpy(&dictSize, input.data() + pos, sizeof(dictSize));
    pos += sizeof(dictSize);

    std::cout << "Dictionary size: " << dictSize << std::endl; // Debug

    // 2. Read character-frequency pairs
    std::unordered_map<char, int> freq;
    std::unordered_map<char, char> decoding;
    for (uint16_t i = 0; i < dictSize; i++) {
        if (pos + sizeof(char) + sizeof(int) > input.size()) {
            throw std::runtime_error("Invalid compressed data: dictionary truncated");
        }
        
        char c = input[pos++];
        int frequency;
        std::memcpy(&frequency, input.data() + pos, sizeof(int));
        pos += sizeof(int);
        
        freq[c] = frequency;
        decoding[static_cast<char>(33 + i)] = c;  // Reverse the encoding
        
        std::cout << "Read char: " << c << " freq: " << frequency << std::endl; // Debug
    }

    // 3. Read original length
    if (pos + sizeof(uint32_t) > input.size()) {
        throw std::runtime_error("Invalid compressed data: missing original length");
    }
    uint32_t originalLength;
    std::memcpy(&originalLength, input.data() + pos, sizeof(originalLength));
    pos += sizeof(uint32_t);

    std::cout << "Original length: " << originalLength << std::endl; // Debug

    // 4. Decode the data
    std::string result;
    result.reserve(originalLength);
    
    while (pos < input.size()) {
        char encoded = input[pos++];
        if (decoding.find(encoded) == decoding.end()) {
            throw std::runtime_error("Invalid encoded character found");
        }
        result += decoding[encoded];
    }

    // 5. Verify length
    if (result.length() != originalLength) {
        throw std::runtime_error("Decoded length mismatch");
    }

    return result;
}

// Verification function
// bool verifyEncoding(const std::string& original) {
//     std::string encoded = haufmann(original);
//     std::string decoded = dehaufmann(encoded);
//     return original == decoded;
// }

int main() {
    try {
        // File paths
        const std::string input_file = "output.txt";
        const std::string output_file = "output2.txt";

        // Read input file
        std::ifstream infile(input_file);
        if (!infile.is_open()) {
            throw std::runtime_error("Could not open input file!");
        }

        // Read mode from first line
        std::string mode;
        infile>>mode;
        if (mode != "compress" && mode != "decompress") {
            throw std::runtime_error("Invalid mode. Use 'compress' or 'decompress'.");
        }

        // Read base64 data from second line
        std::string base64_data;
        infile >> base64_data;
        if (base64_data.empty()) {
            throw std::runtime_error("Empty data in input file!");
        }
        infile.close();

        // Check that base64_data is non-empty and valid
        // If not valid, throw an error early
        // auto decoded_result = td::base64_decode(base64_data);
        // if (decoded_result.size() == 0) {
        //     throw std::runtime_error("Invalid or empty base64 data in input file.");
        // }

        // Process the data
        td::BufferSlice data;
        if (mode == "compress") {
            // std::string huffman_data = haufmann(base64_data);
            // Use our safe deserializer to avoid fatal errors
            data=(td::base64_decode(base64_data));
            // auto root = safe_deserialize_boc(huffman_data);
            td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
            // Now compress
            // data = td::lz4_compress(vm::std_boc_serialize(root, 2).move_as_ok());
            td::BufferSlice serialized = vm::std_boc_serialize(root, 2).move_as_ok();

            data =  td::lz4_compress(serialized);
            // std::string output = haufmann(td::str_base64_encode(data));
            std::ofstream outfile(output_file);
            if (!outfile.is_open()) {
                throw std::runtime_error("Could not open output file!");
            }
            std::string output = td::str_base64_encode(data);
            output=haufmann(output);
            outfile <<output<< std::endl;
            outfile.close();
        } 
        
    
    else {
        try {
            // First decompress
            std::string huffman_data = dehaufmann(base64_data);
            data = td::base64_decode(huffman_data);
    
            // Debug: Check if data is valid after base64 decoding
            if (data.empty()) {
                throw std::runtime_error("Decoded data is empty!");
            }
    
            td::BufferSlice serialized = td::lz4_decompress(data, 2 << 20).move_as_ok();
    
            // Debug: Check if serialized data is valid after decompression
            if (serialized.size() == 0) {
                throw std::runtime_error("Decompressed data is empty!");
            }
    
            auto root = vm::std_boc_deserialize(serialized).move_as_ok();
    
            // Debug: Check if root is valid after deserialization
            // if (!root) {
            //     throw std::runtime_error("Deserialization failed!");
            // }
    
            data = vm::std_boc_serialize(root, 31).move_as_ok();
    
            std::ofstream outfile(output_file);
            if (!outfile.is_open()) {
                throw std::runtime_error("Could not open output file!");
            }
    
            std::string output = td::str_base64_encode(data);
            outfile << output << std::endl;
            outfile.close();
     
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

        // // Write output to file
        // std::ofstream outfile(output_file);
        // if (!outfile.is_open()) {
        //     throw std::runtime_error("Could not open output file!");
        // }
        // std::string output = td::str_base64_encode(data);
        // output=haufmann(output);
        // outfile <<output<< std::endl;
        // outfile.close();

        std::cout << "Operation completed. Output written to " << output_file << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}