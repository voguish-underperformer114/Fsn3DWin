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

const char* fileNodeKindName(const FileNode& node)
{
    switch (node.type) {
    case FileNodeType::Directory:
        return "Folder";
    case FileNodeType::Symlink:
        return "Link";
    case FileNodeType::Error:
        return "Unreadable item";
    case FileNodeType::Other:
        return "Other item";
    case FileNodeType::File:
        break;
    }

    switch (node.category) {
    case FileCategory::Image:
        return "Image file";
    case FileCategory::Source:
        return "Code file";
    case FileCategory::Document:
        return "Document";
    case FileCategory::Audio:
        return "Audio file";
    case FileCategory::Video:
        return "Video file";
    case FileCategory::Archive:
        return "Archive";
    case FileCategory::Executable:
        return "App or executable";
    case FileCategory::System:
        return "System file";
    case FileCategory::Data:
        return "Data file";
    case FileCategory::Error:
        return "Unreadable item";
    case FileCategory::Directory:
        return "Folder";
    case FileCategory::Other:
    default:
        return "File";
    }
}

const char* fileNodeLabelPrefix(const FileNode& node)
{
    switch (node.type) {
    case FileNodeType::Directory:
        return "Folder";
    case FileNodeType::Symlink:
        return "Link";
    case FileNodeType::Error:
        return "Error";
    case FileNodeType::Other:
        return "Item";
    case FileNodeType::File:
        break;
    }

    switch (node.category) {
    case FileCategory::Image:
        return "Image";
    case FileCategory::Source:
        return "Code";
    case FileCategory::Document:
        return "Document";
    case FileCategory::Audio:
        return "Audio";
    case FileCategory::Video:
        return "Video";
    case FileCategory::Archive:
        return "Archive";
    case FileCategory::Executable:
        return "App";
    case FileCategory::System:
        return "System";
    case FileCategory::Data:
        return "Data";
    case FileCategory::Error:
        return "Error";
    case FileCategory::Directory:
        return "Folder";
    case FileCategory::Other:
    default:
        return "File";
    }
}
