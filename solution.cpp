#include "td/utils/buffer.h"
#include "td/utils/lz4.h" // TON's LZ4 library
#include "vm/boc.h"       // For TON serialization/deserialization
#include "common/util.h"
#include "vm/cellslice.h"
#include "td/utils/misc.h"
#include "td/utils/Gzip.h"
#include <fstream>
#include <iostream>

// Compress function using LZ4 with block splitting
td::BufferSlice compress(td::Slice data) {
    try {
        td::Ref<vm::Cell> root = vm::std_boc_deserialize(data).move_as_ok();
        td::BufferSlice serialized = vm::std_boc_serialize(root, 0).move_as_ok();

        // Define optimal chunk size for data
        const std::size_t chunk_size = 128 * 1024; // Experiment with this value
        td::BufferBuilder final_compressed_builder;

        for (std::size_t i = 0; i < serialized.size(); i += chunk_size) {
            td::Slice chunk(serialized.data() + i, std::min(chunk_size, serialized.size() - i));
            auto compressed_chunk = td::lz4_compress(chunk);

            if (compressed_chunk.empty()) {
                throw std::runtime_error("Chunk compression failed!");
            }
            final_compressed_builder.append(std::move(compressed_chunk));
        }

        // Finalize the compressed buffer
        return final_compressed_builder.extract();
    } catch (const std::exception &e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        return {};
    }
}

// Decompress function using LZ4
td::BufferSlice decompress(td::Slice data) {
    try {
        td::BufferSlice decompressed = td::lz4_decompress(data, 128 * 1024 * 2).move_as_ok();
        auto root = vm::std_boc_deserialize(decompressed).move_as_ok();
        return vm::std_boc_serialize(root, 31).move_as_ok();
    } catch (const std::exception &e) {
        std::cerr << "Decompression failed: " << e.what() << std::endl;
        return {};
    }
}

int main() {
    try {
        const std::string input_file = "input.txt";
        const std::string output_file = "output.txt";

        std::ifstream infile(input_file);
        if (!infile.is_open()) {
            throw std::runtime_error("Could not open input file!");
        }

        std::string mode, base64_data;
        infile >> mode >> base64_data;
        infile.close();

        if (mode != "compress" && mode != "decompress") {
            throw std::runtime_error("Invalid mode. Use 'compress' or 'decompress'.");
        }

        td::BufferSlice data = td::base64_decode(base64_data);

        if (mode == "compress") {
            data = compress(data);
            std::cout << "Compressed size: " << data.size() << " bytes" << std::endl;
        } else {
            data = decompress(data);
            std::cout << "Decompressed size: " << data.size() << " bytes" << std::endl;
        }

        std::ofstream outfile(output_file);
        if (!outfile.is_open()) {
            throw std::runtime_error("Could not open output file!");
        }

        std::string output = td::str_base64_encode(data);
        outfile << output << std::endl;
        outfile.close();

        std::cout << "Operation completed. Output written to " << output_file << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
