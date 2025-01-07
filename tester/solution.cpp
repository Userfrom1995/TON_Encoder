#include <iostream>
#include "td/utils/buffer.h" 
#include "td/utils/lz4.h" // TON's LZ4 library
#include "vm/boc.h" // For TON serialization/deserialization
#include "common/util.h" 
#include "vm/cellslice.h" 
#include "td/utils/misc.h"
#include "td/utils/Gzip.h"

// Compress function using gzip
td::BufferSlice compress(td::Slice data) {
    try {
        double max_compression_ratio = 10.0; // Adjust as needed
        auto compressed = td::gzencode(data, max_compression_ratio);
        if (compressed.empty()) {
            throw std::runtime_error("Gzip compression returned empty data.");
        }
        return compressed;
    } catch (const std::exception& e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        return {};
    }
}

// Decompress function using gzip
td::BufferSlice decompress(td::Slice data) {
    try {
        auto decompressed = td::gzdecode(data);
        if (decompressed.empty()) {
            throw std::runtime_error("Gzip decompression returned empty data.");
        }
        return decompressed;
    } catch (const std::exception& e) {
        std::cerr << "Decompression failed: " << e.what() << std::endl;
        return {};
    }
}

int main() {
    try {
        // File paths
        // const std::string input_file = "output.txt";
        // const std::string output_file = "output2.txt";

        // // Read input file
        // std::ifstream infile(input_file);
        // if (!infile.is_open()) {
        //     throw std::runtime_error("Could not open input file!");
        // }

        // Read mode from first line
        std::string mode;
        // infile >> mode;
        // if (mode != "compress" && mode != "decompress") {
        //     throw std::runtime_error("Invalid mode. Use 'compress' or 'decompress'.");
        // }
        std::cin>>mode;

        // Read base64 data from second line
        std::string base64_data;
        std::cin>>base64_data;
        // infile >> base64_data;
        // if (base64_data.empty()) {
        //     throw std::runtime_error("Empty data in input file!");
        // }
        // infile.close();

        // // Decode Base64 input
        td::BufferSlice data = td::base64_decode(base64_data);
        // std::cout << "Input size: " << data.size() << " bytes" << std::endl;

        // Process data based on the mode
        if (mode == "compress") {
            data = compress(data);
            // std::cout << "Compressed size: " << data.size() << " bytes" << std::endl;
        } else {
            data = decompress(data);
            // std::cout << "Decompressed size: " << data.size() << " bytes" << std::endl;
        }

        // Encode the result to Base64 and write to output file
        // std::ofstream outfile(output_file);
        // if (!outfile.is_open()) {
        //     throw std::runtime_error("Could not open output file!");
        // }

        std::string output = td::str_base64_encode(data);
        // outfile << output << std::endl;
        // outfile.close();
        std::cout << output << std::endl;

        // std::cout << "Operation completed. Output written to " << output_file << std::endl;
        // return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
