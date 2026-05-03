#pragma once

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

using FileNodeId = std::uint32_t;
constexpr FileNodeId InvalidFileNodeId = std::numeric_limits<FileNodeId>::max();

enum class FileNodeType {
    Directory,
    File,
    Symlink,
    Other,
    Error,
};

enum class FileCategory {
    Directory,
    Source,
    Document,
    Image,
    Audio,
    Video,
    Archive,
    Executable,
    System,
    Data,
    Other,
    Error,
};

struct FileNode {
    FileNodeId id = InvalidFileNodeId;
    FileNodeId parentId = InvalidFileNodeId;
    std::string name;
    std::filesystem::path fullPath;
    FileNodeType type = FileNodeType::Other;
    std::string extension;
    std::uintmax_t sizeBytes = 0;
    bool sizeKnown = false;
    std::uint32_t depth = 0;
    std::vector<FileNodeId> childIds;
    FileCategory category = FileCategory::Other;
    std::string warning;
};

const char* fileNodeTypeName(FileNodeType type);
const char* fileCategoryName(FileCategory category);
const char* fileNodeKindName(const FileNode& node);
const char* fileNodeLabelPrefix(const FileNode& node);
