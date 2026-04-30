#include "filesystem/FileScanner.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <algorithm>
#include <chrono>
#include <cctype>
#include <limits>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace {
constexpr std::size_t MaxStoredWarnings = 32;
constexpr std::size_t MaxStoredErrors = 16;

bool shouldCancel(const std::atomic_bool* cancelRequested)
{
    return cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed);
}

void syncProgress(FileScanProgress* progress, const FileScanResult& result)
{
    if (progress == nullptr) {
        return;
    }

    progress->nodesScanned.store(result.counts.nodes, std::memory_order_relaxed);
    progress->directoriesScanned.store(result.counts.directories, std::memory_order_relaxed);
    progress->warnings.store(result.warnings.size(), std::memory_order_relaxed);
    progress->errors.store(result.errors.size() + result.counts.errors, std::memory_order_relaxed);
}

std::string pathString(const fs::path& path)
{
    try {
        return path.string();
    } catch (...) {
        return "<path>";
    }
}

std::string lowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

void appendLimited(std::vector<std::string>& messages, std::size_t limit, std::string message)
{
    if (messages.size() < limit) {
        messages.push_back(std::move(message));
    }
}

void appendWarning(FileScanResult& result, FileScanProgress* progress, std::string message)
{
    appendLimited(result.warnings, MaxStoredWarnings, std::move(message));
    syncProgress(progress, result);
}

void appendError(FileScanResult& result, FileScanProgress* progress, std::string message)
{
    appendLimited(result.errors, MaxStoredErrors, std::move(message));
    syncProgress(progress, result);
}

std::string describeError(const fs::path& path, const std::error_code& error)
{
    std::ostringstream stream;
    stream << pathString(path) << ": " << error.message();
    return stream.str();
}

bool isHiddenPath(const fs::path& path)
{
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_HIDDEN) != 0U) {
        return true;
    }
#endif

    const std::string filename = path.filename().string();
    return !filename.empty() && filename.front() == '.';
}

FileNodeType nodeTypeFromStatus(const fs::file_status& status)
{
    if (fs::is_symlink(status)) {
        return FileNodeType::Symlink;
    }

    if (fs::is_directory(status)) {
        return FileNodeType::Directory;
    }

    if (fs::is_regular_file(status)) {
        return FileNodeType::File;
    }

    return FileNodeType::Other;
}

FileCategory categoryFor(FileNodeType type, const std::string& extension)
{
    if (type == FileNodeType::Error) {
        return FileCategory::Error;
    }

    if (type == FileNodeType::Directory) {
        return FileCategory::Directory;
    }

    const std::string ext = lowerCopy(extension);

    if (ext == ".cpp" || ext == ".c" || ext == ".hpp" || ext == ".h" || ext == ".cs" ||
        ext == ".js" || ext == ".ts" || ext == ".py" || ext == ".rs" || ext == ".glsl" ||
        ext == ".cmake" || ext == ".json" || ext == ".md") {
        return FileCategory::Source;
    }

    if (ext == ".txt" || ext == ".pdf" || ext == ".doc" || ext == ".docx" || ext == ".rtf") {
        return FileCategory::Document;
    }

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".webp") {
        return FileCategory::Image;
    }

    if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg") {
        return FileCategory::Audio;
    }

    if (ext == ".mp4" || ext == ".mov" || ext == ".mkv" || ext == ".avi" || ext == ".webm") {
        return FileCategory::Video;
    }

    if (ext == ".zip" || ext == ".7z" || ext == ".rar" || ext == ".tar" || ext == ".gz") {
        return FileCategory::Archive;
    }

    if (ext == ".exe" || ext == ".dll" || ext == ".bat" || ext == ".cmd" || ext == ".ps1" || ext == ".msi") {
        return FileCategory::Executable;
    }

    if (ext == ".sys" || ext == ".ini" || ext == ".cfg" || ext == ".log") {
        return FileCategory::System;
    }

    if (type == FileNodeType::File) {
        return FileCategory::Data;
    }

    return FileCategory::Other;
}

void updateCounts(FileScanCounts& counts, const FileNode& node)
{
    ++counts.nodes;

    switch (node.type) {
    case FileNodeType::Directory:
        ++counts.directories;
        break;
    case FileNodeType::File:
        ++counts.files;
        break;
    case FileNodeType::Symlink:
        ++counts.symlinks;
        break;
    case FileNodeType::Error:
        ++counts.errors;
        break;
    case FileNodeType::Other:
    default:
        ++counts.other;
        break;
    }

    if (node.sizeKnown) {
        counts.knownBytes += node.sizeBytes;
    }
}

FileNodeId addNode(
    FileScanResult& result,
    FileNode node,
    std::size_t maxNodes,
    FileScanProgress* progress)
{
    if (result.nodes.size() >= maxNodes) {
        result.truncated = true;
        syncProgress(progress, result);
        return InvalidFileNodeId;
    }

    node.id = static_cast<FileNodeId>(result.nodes.size());
    const FileNodeId parentId = node.parentId;
    result.nodes.push_back(std::move(node));

    if (parentId != InvalidFileNodeId && parentId < result.nodes.size()) {
        result.nodes[parentId].childIds.push_back(result.nodes.back().id);
    }

    updateCounts(result.counts, result.nodes.back());
    syncProgress(progress, result);
    return result.nodes.back().id;
}

FileNode makeNode(const fs::path& path, FileNodeId parentId, std::uint32_t depth, FileNodeType type)
{
    FileNode node;
    node.parentId = parentId;
    node.name = path.filename().empty() ? pathString(path) : path.filename().string();
    node.fullPath = path;
    node.type = type;
    node.extension = type == FileNodeType::File ? lowerCopy(path.extension().string()) : std::string();
    node.depth = depth;
    node.category = categoryFor(type, node.extension);

    if (type == FileNodeType::File) {
        std::error_code sizeError;
        const std::uintmax_t size = fs::file_size(path, sizeError);
        if (!sizeError) {
            node.sizeBytes = size;
            node.sizeKnown = true;
        }
    }

    return node;
}

fs::path canonicalKey(const fs::path& path)
{
    std::error_code error;
    fs::path key = fs::weakly_canonical(path, error);
    if (!error) {
        return key;
    }

    key = fs::absolute(path, error);
    if (!error) {
        return key;
    }

    return path;
}

bool markVisitedDirectory(const fs::path& path, std::set<fs::path>& visited)
{
    const fs::path key = canonicalKey(path);
    const auto [_, inserted] = visited.insert(key);
    return inserted;
}

bool isDirectoryTarget(const fs::directory_entry& entry)
{
    std::error_code error;
    const bool isDirectory = entry.is_directory(error);
    return !error && isDirectory;
}

void scanDirectory(
    const fs::path& directoryPath,
    FileNodeId parentId,
    std::uint32_t depth,
    const FileScanOptions& options,
    FileScanResult& result,
    std::set<fs::path>& visited,
    std::size_t maxNodes,
    FileScanProgress* progress,
    const std::atomic_bool* cancelRequested)
{
    if (depth >= options.maxDepth || result.truncated || shouldCancel(cancelRequested)) {
        return;
    }

    fs::directory_options directoryOptions = fs::directory_options::skip_permission_denied;
    if (options.followSymlinks) {
        directoryOptions |= fs::directory_options::follow_directory_symlink;
    }

    std::error_code iteratorError;
    fs::directory_iterator iterator(directoryPath, directoryOptions, iteratorError);
    if (iteratorError) {
        ++result.counts.errors;
        appendWarning(result, progress, describeError(directoryPath, iteratorError));
        return;
    }

    const fs::directory_iterator end;
    for (; iterator != end && !result.truncated && !shouldCancel(cancelRequested); iterator.increment(iteratorError)) {
        if (iteratorError) {
            ++result.counts.errors;
            appendWarning(result, progress, describeError(directoryPath, iteratorError));
            iteratorError.clear();
            continue;
        }

        const fs::directory_entry& entry = *iterator;
        const fs::path childPath = entry.path();

        if (!options.includeHidden && isHiddenPath(childPath)) {
            continue;
        }

        std::error_code statusError;
        fs::file_status status = entry.symlink_status(statusError);
        FileNodeType type = FileNodeType::Other;

        if (statusError) {
            type = FileNodeType::Error;
        } else {
            type = nodeTypeFromStatus(status);
        }

        FileNode node = makeNode(childPath, parentId, depth + 1U, type);
        if (statusError) {
            node.warning = statusError.message();
            appendWarning(result, progress, describeError(childPath, statusError));
        }

        const FileNodeId childId = addNode(result, std::move(node), maxNodes, progress);
        if (childId == InvalidFileNodeId) {
            break;
        }

        const bool canRecurseDirectory = type == FileNodeType::Directory ||
            (type == FileNodeType::Symlink && options.followSymlinks && isDirectoryTarget(entry));

        if (canRecurseDirectory) {
            if (!markVisitedDirectory(childPath, visited)) {
                appendWarning(result, progress, "Skipped already visited directory: " + pathString(childPath));
                continue;
            }

            scanDirectory(childPath, childId, depth + 1U, options, result, visited, maxNodes, progress, cancelRequested);
        }
    }
}
}

FileScanResult FileScanner::scan(const FileScanOptions& options) const
{
    return scan(options, nullptr, nullptr);
}

FileScanResult FileScanner::scan(
    const FileScanOptions& options,
    FileScanProgress* progress,
    const std::atomic_bool* cancelRequested) const
{
    using Clock = std::chrono::steady_clock;

    const auto started = Clock::now();
    FileScanResult result;
    syncProgress(progress, result);

    try {
        const std::size_t maxNodes = std::max<std::size_t>(1, std::min<std::size_t>(
            options.maxNodes,
            static_cast<std::size_t>(std::numeric_limits<FileNodeId>::max() - 1U)));

        std::error_code pathError;
        fs::path rootPath = options.rootPath.empty() ? fs::current_path(pathError) : options.rootPath;
        if (pathError) {
            appendError(result, progress, "Unable to read current directory: " + pathError.message());
            result.elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();
            return result;
        }

        fs::path absoluteRoot = fs::absolute(rootPath, pathError);
        if (!pathError) {
            rootPath = absoluteRoot;
        } else {
            appendWarning(result, progress, describeError(rootPath, pathError));
            pathError.clear();
        }

        fs::file_status rootStatus = fs::symlink_status(rootPath, pathError);
        if (pathError) {
            appendError(result, progress, describeError(rootPath, pathError));
            result.elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();
            return result;
        }

        if (!fs::exists(rootStatus)) {
            appendError(result, progress, "Root path does not exist: " + pathString(rootPath));
            result.elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();
            return result;
        }

        FileNodeType rootType = nodeTypeFromStatus(rootStatus);
        FileNode rootNode = makeNode(rootPath, InvalidFileNodeId, 0, rootType);
        const FileNodeId rootId = addNode(result, std::move(rootNode), maxNodes, progress);

        if (rootId != InvalidFileNodeId) {
            std::set<fs::path> visited;
            const bool rootCanRecurse = rootType == FileNodeType::Directory ||
                (rootType == FileNodeType::Symlink && options.followSymlinks);

            if (rootCanRecurse && markVisitedDirectory(rootPath, visited)) {
                scanDirectory(rootPath, rootId, 0, options, result, visited, maxNodes, progress, cancelRequested);
            }
        }

        if (shouldCancel(cancelRequested)) {
            appendWarning(result, progress, "Scan canceled by user.");
        }
    } catch (const std::exception& error) {
        appendError(result, progress, std::string("Scan failed: ") + error.what());
    } catch (...) {
        appendError(result, progress, "Scan failed with an unknown error.");
    }

    result.elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();
    syncProgress(progress, result);
    return result;
}

const char* fileScanStateName(FileScanState state)
{
    switch (state) {
    case FileScanState::Idle:
        return "idle";
    case FileScanState::Scanning:
        return "scanning";
    case FileScanState::CancelRequested:
        return "cancel requested";
    case FileScanState::Complete:
        return "complete";
    case FileScanState::Failed:
    default:
        return "failed";
    }
}
