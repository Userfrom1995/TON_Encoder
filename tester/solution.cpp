#include <iostream>
#include <fstream>
#include <unordered_map>
#include "td/utils/lz4.h"
#include "td/utils/base64.h"
#include "vm/boc.h"

std::unordered_map<std::string, char> pattern_map;

std::string find_and_replace_patterns(const std::string& data) {
    std::string modified_data = data;
    size_t pattern_id = 0;

    for (size_t i = 0; i < modified_data.size() - 1; ++i) {
        std::string pattern = modified_data.substr(i, 2);
        if (pattern_map.find(pattern) == pattern_map.end()) {
            pattern_map[pattern] = 'A' + (pattern_id % 26); 
            pattern_id++;
        }
        modified_data.replace(i, 2, 1, pattern_map[pattern]);
    }
    return modified_data;
}

std::string reverse_pattern_replacement(const std::string& compressed_data) {
    std::string decompressed_data = compressed_data;
    for (const auto& entry : pattern_map) {
        decompressed_data.replace(decompressed_data.find(entry.second), 1, entry.first);
    }
    return decompressed_data;
}

td::BufferSlice preprocess(td::Slice data) {
    auto root = vm::std_boc_deserialize(data).move_as_ok();
    return vm::std_boc_serialize(root, 0).move_as_ok();
}

td::BufferSlice compress(td::Slice data) {
    if (data.empty()) {
        return td::BufferSlice();
    }

    td::BufferSlice preprocessed = preprocess(data);
    std::string preprocessed_str(reinterpret_cast<const char*>(preprocessed.data()), preprocessed.size());

    std::string compressed_str = find_and_replace_patterns(preprocessed_str);

    return td::lz4_compress(td::BufferSlice(compressed_str.c_str(), compressed_str.size()));
}

td::BufferSlice decompress(td::Slice data) {
    if (data.empty()) {
        return td::BufferSlice();
    }

    const size_t max_size = 2 << 20;

    td::BufferSlice decompressed = td::lz4_decompress(data, max_size).move_as_ok();
    std::string decompressed_str(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());

    std::string restored_str = reverse_pattern_replacement(decompressed_str);

    td::BufferSlice restored_data(restored_str.c_str(), restored_str.size());
    auto root = vm::std_boc_deserialize(restored_data).move_as_ok();
    return vm::std_boc_serialize(root, 63).move_as_ok(); // Ensure that the '63' argument is correct
}

int main() {
    // Open input and output files
    std::ifstream input_file("input.txt");
    std::ofstream output_file("output.txt");

    if (!input_file.is_open() || !output_file.is_open()) {
        std::cerr << "Error opening files!" << std::endl;
        return 1;
    }

    std::string mode;
    std::getline(input_file, mode);  // Read the first line to determine the mode

    if (mode != "compress" && mode != "decompress") {
        std::cerr << "Invalid mode" << std::endl;
        return 1;
    }

    std::string base64_data;
    std::getline(input_file, base64_data);  // Read the second line for the base64 encoded data

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

    // Write the result to the output file
    output_file << td::base64_encode(data) << std::endl;

    input_file.close();
    output_file.close();

    std::cout << "Process completed. Output saved to output.txt" << std::endl;

    return 0;
}
