#include <iostream>
#include <vector>
#include <cstring>
#include "td/utils/lz4.h"
#include "td/utils/base64.h"

// Define the Cell structure
struct Cell {
    std::vector<uint8_t> data;
    std::vector<uint16_t> refs;
};

// Serialize a single cell
std::vector<uint8_t> serialize_cell(const Cell& cell) {
    std::vector<uint8_t> serialized;

    serialized.push_back(static_cast<uint8_t>(cell.data.size()));
    serialized.insert(serialized.end(), cell.data.begin(), cell.data.end());

    serialized.push_back(static_cast<uint8_t>(cell.refs.size()));
    for (uint16_t ref : cell.refs) {
        serialized.push_back(ref & 0xFF);
        serialized.push_back((ref >> 8) & 0xFF);
    }

    return serialized;
}

// Serialize a full block (Bag of Cells)
std::vector<uint8_t> custom_serialize(const std::vector<Cell>& cells) {
    std::vector<uint8_t> serialized;

    serialized.push_back(static_cast<uint8_t>(cells.size()));
    for (const Cell& cell : cells) {
        auto cell_data = serialize_cell(cell);
        serialized.insert(serialized.end(), cell_data.begin(), cell_data.end());
    }

    return serialized;
}

// Deserialize a single cell
Cell deserialize_cell(const uint8_t* data, size_t& offset) {
    Cell cell;

    uint8_t data_size = data[offset++];
    cell.data.assign(data + offset, data + offset + data_size);
    offset += data_size;

    uint8_t num_refs = data[offset++];
    for (size_t i = 0; i < num_refs; ++i) {
        uint16_t ref = data[offset] | (data[offset + 1] << 8);
        cell.refs.push_back(ref);
        offset += 2;
    }

    return cell;
}

// Deserialize a full block (Bag of Cells)
std::vector<Cell> custom_deserialize(const std::vector<uint8_t>& serialized) {
    size_t offset = 0;
    std::vector<Cell> cells;

    uint8_t num_cells = serialized[offset++];
    for (size_t i = 0; i < num_cells; ++i) {
        cells.push_back(deserialize_cell(serialized.data(), offset));
    }

    return cells;
}

// Preprocess: Custom serialization
std::vector<uint8_t> preprocess(const std::vector<uint8_t>& input) {
    auto cells = custom_deserialize(input);
    return custom_serialize(cells);
}

// Compress: Apply LZ4 on preprocessed data
std::vector<uint8_t> compress(const std::vector<uint8_t>& input) {
    if (input.empty()) {
        return {};
    }
    auto preprocessed = preprocess(input);
    auto compressed = td::lz4_compress(td::Slice(reinterpret_cast<const char*>(preprocessed.data()), preprocessed.size()));
    return std::vector<uint8_t>(compressed.as_slice().begin(), compressed.as_slice().end());
}

// Decompress: Reverse LZ4 and custom serialization
std::vector<uint8_t> decompress(const std::vector<uint8_t>& input) {
    if (input.empty()) {
        return {};
    }
    constexpr size_t max_size = 2 << 20; // 2MB safety limit
    
    // Just decompress the LZ4 data and return it directly
    auto decompressed = td::lz4_decompress(
        td::Slice(reinterpret_cast<const char*>(input.data()), input.size()), 
        max_size
    ).move_as_ok();
    
    // Convert to vector<uint8_t> and return
    return std::vector<uint8_t>(
        decompressed.as_slice().begin(), 
        decompressed.as_slice().end()
    );
}

// Main logic
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

    auto decoded_data = td::base64_decode(base64_data).move_as_ok();
    std::string data(decoded_data.data(), decoded_data.size());
    std::vector<uint8_t> result;

    if (mode == "compress") {
        result = compress(std::vector<uint8_t>(data.data(), data.data() + data.size()));
    } else {
        result = decompress(std::vector<uint8_t>(data.data(), data.data() + data.size()));
    }

    std::cout << td::base64_encode(td::Slice(reinterpret_cast<const char*>(result.data()), result.size())) << std::endl;
    return 0;
}
