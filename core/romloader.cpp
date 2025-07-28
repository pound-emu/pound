// romloader.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

class RomLoader {
public:
    explicit RomLoader(const std::string& path)
        : romPath(path) {}

    bool load() {
        if (!std::filesystem::exists(romPath)) {
            std::cerr << "ROM file not found: " << romPath << "\n";
            return false;
        }

        std::ifstream file(romPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open ROM file.\n";
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        romData.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(romData.data()), size)) {
            std::cerr << "Failed to read ROM file.\n";
            return false;
        }

        std::cout << "ROM loaded successfully: " << romPath << " (" << size << " bytes)\n";

        // Here you would parse headers (e.g., PFS0, NRO, NCA) and extract content
        return true;
    }

    const std::vector<uint8_t>& getData() const {
        return romData;
    }

    std::string getRomName() const {
        return std::filesystem::path(romPath).filename().string();
    }

private:
    std::string romPath;
    std::vector<uint8_t> romData;
};
