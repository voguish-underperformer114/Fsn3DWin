#pragma once

#include "filesystem/FileScanner.hpp"
#include "scene/SceneNode.hpp"

#include <string>
#include <vector>

enum class FileWorldLayoutMode {
    FilesystemCity = 0,
    CategoryDistricts,
    SizeSkyline,
};

enum class SizeEmphasisMode {
    Normal = 0,
    Skyline,
};

struct FileWorldSettings {
    float spacing = 1.45f;
    float heightScale = 0.42f;
    float directoryTowerScale = 1.0f;
    float maxHeight = 18.0f;
    float citySpread = 1.0f;
    FileWorldLayoutMode layoutMode = FileWorldLayoutMode::FilesystemCity;
    SizeEmphasisMode sizeEmphasis = SizeEmphasisMode::Normal;
};

struct FileWorldFilters {
    std::string nameSubstring;
    std::string extensionSubstring;
    bool showDirectories = true;
    bool showCode = true;
    bool showImages = true;
    bool showVideo = true;
    bool showAudio = true;
    bool showDocuments = true;
    bool showArchives = true;
    bool showExecutables = true;
    bool showOther = true;
};

struct FileWorldLayout {
    std::vector<SceneNode> nodes;
    std::size_t visibleCount = 0;
};

class FileWorldBuilder {
public:
    FileWorldLayout build(const FileScanResult& scanResult, const FileWorldSettings& settings) const;
};

const char* fileWorldLayoutModeName(FileWorldLayoutMode mode);
const char* sizeEmphasisModeName(SizeEmphasisMode mode);
bool fileCategoryAllowed(FileCategory category, const FileWorldFilters& filters);
