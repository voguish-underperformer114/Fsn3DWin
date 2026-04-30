#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct LoadedImage {
    bool ok = false;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba;
    std::string error;
};

LoadedImage loadImageFileRgba(const std::filesystem::path& path);
