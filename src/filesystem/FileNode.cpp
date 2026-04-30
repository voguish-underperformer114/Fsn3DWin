#include "filesystem/FileNode.hpp"

const char* fileNodeTypeName(FileNodeType type)
{
    switch (type) {
    case FileNodeType::Directory:
        return "Directory";
    case FileNodeType::File:
        return "File";
    case FileNodeType::Symlink:
        return "Symlink";
    case FileNodeType::Error:
        return "Error";
    case FileNodeType::Other:
    default:
        return "Other";
    }
}

const char* fileCategoryName(FileCategory category)
{
    switch (category) {
    case FileCategory::Directory:
        return "Directory";
    case FileCategory::Source:
        return "Source";
    case FileCategory::Document:
        return "Document";
    case FileCategory::Image:
        return "Image";
    case FileCategory::Audio:
        return "Audio";
    case FileCategory::Video:
        return "Video";
    case FileCategory::Archive:
        return "Archive";
    case FileCategory::Executable:
        return "Executable";
    case FileCategory::System:
        return "System";
    case FileCategory::Data:
        return "Data";
    case FileCategory::Error:
        return "Error";
    case FileCategory::Other:
    default:
        return "Other";
    }
}
