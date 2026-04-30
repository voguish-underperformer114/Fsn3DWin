#pragma once

#include "filesystem/FileNode.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

enum class FileScanState {
    Idle,
    Scanning,
    CancelRequested,
    Complete,
    Failed,
};

struct FileScanOptions {
    std::filesystem::path rootPath;
    std::uint32_t maxDepth = 4;
    std::size_t maxNodes = 1500;
    bool includeHidden = false;
    bool followSymlinks = false;
};

struct FileScanProgress {
    std::atomic_size_t nodesScanned{0};
    std::atomic_size_t directoriesScanned{0};
    std::atomic_size_t warnings{0};
    std::atomic_size_t errors{0};
};

struct FileScanCounts {
    std::size_t nodes = 0;
    std::size_t directories = 0;
    std::size_t files = 0;
    std::size_t symlinks = 0;
    std::size_t other = 0;
    std::size_t errors = 0;
    std::uintmax_t knownBytes = 0;
};

struct FileScanResult {
    std::vector<FileNode> nodes;
    FileScanCounts counts;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool truncated = false;
    double elapsedSeconds = 0.0;
};

class FileScanner {
public:
    FileScanResult scan(const FileScanOptions& options) const;
    FileScanResult scan(
        const FileScanOptions& options,
        FileScanProgress* progress,
        const std::atomic_bool* cancelRequested) const;
};

const char* fileScanStateName(FileScanState state);
