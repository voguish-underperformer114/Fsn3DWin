#include "scene/FileWorldBuilder.hpp"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <array>
#include <unordered_map>
#include <vector>

namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr int CategoryDistrictCount = 9;

glm::vec4 categoryColor(FileCategory category)
{
    switch (category) {
    case FileCategory::Directory:
        return glm::vec4(0.10f, 0.95f, 0.82f, 1.0f);
    case FileCategory::Source:
        return glm::vec4(0.20f, 0.72f, 1.0f, 1.0f);
    case FileCategory::Document:
        return glm::vec4(0.95f, 0.88f, 0.35f, 1.0f);
    case FileCategory::Image:
        return glm::vec4(1.0f, 0.35f, 0.82f, 1.0f);
    case FileCategory::Audio:
        return glm::vec4(0.58f, 0.38f, 1.0f, 1.0f);
    case FileCategory::Video:
        return glm::vec4(1.0f, 0.52f, 0.22f, 1.0f);
    case FileCategory::Archive:
        return glm::vec4(0.62f, 1.0f, 0.35f, 1.0f);
    case FileCategory::Executable:
        return glm::vec4(1.0f, 0.18f, 0.26f, 1.0f);
    case FileCategory::System:
        return glm::vec4(0.82f, 0.92f, 1.0f, 1.0f);
    case FileCategory::Data:
        return glm::vec4(0.32f, 1.0f, 0.92f, 1.0f);
    case FileCategory::Error:
        return glm::vec4(1.0f, 0.08f, 0.08f, 1.0f);
    case FileCategory::Other:
    default:
        return glm::vec4(0.62f, 0.72f, 0.86f, 1.0f);
    }
}

float fileHeight(const FileNode& node, const FileWorldSettings& settings)
{
    if (!node.sizeKnown) {
        return 0.55f;
    }

    const double bytes = static_cast<double>(node.sizeBytes) + 1.0;
    const float emphasis = settings.sizeEmphasis == SizeEmphasisMode::Skyline ? 2.15f : 1.0f;
    const float height = 0.45f + static_cast<float>(std::log2(bytes) * 0.18) * settings.heightScale * emphasis;
    return std::clamp(height, 0.35f, settings.maxHeight);
}

float directoryHeight(const FileNode& node, const FileScanResult& scanResult, const FileWorldSettings& settings)
{
    const float childWeight = static_cast<float>(node.childIds.size()) * 0.16f;
    const float depthWeight = static_cast<float>(node.depth) * 0.35f;
    float fileWeight = 0.0f;

    for (FileNodeId childId : node.childIds) {
        if (childId < scanResult.nodes.size() && scanResult.nodes[childId].type == FileNodeType::File) {
            fileWeight += scanResult.nodes[childId].sizeKnown
                ? std::min(1.6f, static_cast<float>(std::log2(static_cast<double>(scanResult.nodes[childId].sizeBytes) + 1.0) * 0.035))
                : 0.08f;
        }
    }

    const float skylineWeight = settings.sizeEmphasis == SizeEmphasisMode::Skyline ? 0.82f : 1.0f;
    const float baseHeight = node.depth == 0 ? 8.5f : 2.2f;
    return std::clamp((baseHeight + childWeight + depthWeight + fileWeight) * settings.directoryTowerScale * skylineWeight, 1.2f, settings.maxHeight);
}

glm::vec3 nodeScale(const FileNode& node, const FileScanResult& scanResult, const FileWorldSettings& settings)
{
    if (node.type == FileNodeType::Directory) {
        const float width = node.depth == 0 ? 2.8f : std::clamp(1.0f + static_cast<float>(node.childIds.size()) * 0.025f, 0.9f, 2.2f);
        return glm::vec3(width, directoryHeight(node, scanResult, settings), width);
    }

    if (node.type == FileNodeType::Symlink) {
        return glm::vec3(0.78f, 1.35f, 0.78f);
    }

    if (node.type == FileNodeType::Error) {
        return glm::vec3(0.85f, 0.85f, 0.85f);
    }

    const float width = node.type == FileNodeType::File
        ? (settings.sizeEmphasis == SizeEmphasisMode::Skyline ? 0.54f : 0.62f)
        : 0.72f;
    return glm::vec3(width, fileHeight(node, settings), width);
}

float nodeSortSize(const FileNode& node)
{
    if (node.sizeKnown) {
        return static_cast<float>(std::log2(static_cast<double>(node.sizeBytes) + 1.0));
    }

    return node.type == FileNodeType::Directory ? static_cast<float>(node.childIds.size()) * 0.55f : 0.0f;
}

int categoryDistrict(FileCategory category)
{
    switch (category) {
    case FileCategory::Directory:
        return 0;
    case FileCategory::Source:
        return 1;
    case FileCategory::Image:
        return 2;
    case FileCategory::Video:
        return 3;
    case FileCategory::Audio:
        return 4;
    case FileCategory::Document:
        return 5;
    case FileCategory::Archive:
        return 6;
    case FileCategory::Executable:
        return 7;
    case FileCategory::System:
    case FileCategory::Data:
    case FileCategory::Other:
    case FileCategory::Error:
    default:
        return 8;
    }
}

SceneNode makeSceneNode(
    SceneNodeId sceneId,
    const FileNode& fileNode,
    const glm::vec3& scale,
    const glm::vec3& groundPosition)
{
    SceneNode sceneNode;
    sceneNode.sceneId = sceneId;
    sceneNode.sourceFileNodeId = fileNode.id;
    sceneNode.position = glm::vec3(groundPosition.x, scale.y * 0.5f, groundPosition.z);
    sceneNode.scale = scale;
    sceneNode.color = categoryColor(fileNode.category);
    sceneNode.category = fileNode.category;
    sceneNode.label = fileNode.name;
    sceneNode.bounds = makeAabb(sceneNode.position, sceneNode.scale);
    sceneNode.visible = true;
    return sceneNode;
}

float stableJitter(FileNodeId id)
{
    const unsigned int value = static_cast<unsigned int>(id * 1103515245U + 12345U);
    return static_cast<float>(value % 1000U) / 1000.0f;
}

std::vector<FileNodeId> sortedChildren(const FileNode& parent, const FileScanResult& scanResult)
{
    std::vector<FileNodeId> children = parent.childIds;
    std::stable_sort(children.begin(), children.end(), [&scanResult](FileNodeId lhs, FileNodeId rhs) {
        const FileNode& a = scanResult.nodes[lhs];
        const FileNode& b = scanResult.nodes[rhs];
        if (a.type != b.type) {
            return a.type == FileNodeType::Directory;
        }

        if (a.category != b.category) {
            return static_cast<int>(a.category) < static_cast<int>(b.category);
        }

        return a.name < b.name;
    });
    return children;
}

void layoutChildren(
    FileNodeId parentId,
    const glm::vec3& parentGroundPosition,
    const FileScanResult& scanResult,
    const FileWorldSettings& settings,
    FileWorldLayout& layout,
    std::unordered_map<FileNodeId, SceneNodeId>& sceneIds)
{
    if (parentId >= scanResult.nodes.size()) {
        return;
    }

    const FileNode& parent = scanResult.nodes[parentId];
    const std::vector<FileNodeId> children = sortedChildren(parent, scanResult);
    if (children.empty()) {
        return;
    }

    const float depth = static_cast<float>(parent.depth + 1U);
    const float ringRadius = settings.spacing * settings.citySpread *
        (2.2f + std::sqrt(static_cast<float>(children.size())) * 0.78f + depth * 1.05f);
    const float streetOffset = settings.spacing * 0.58f;

    for (std::size_t index = 0; index < children.size(); ++index) {
        const FileNodeId childId = children[index];
        if (childId >= scanResult.nodes.size()) {
            continue;
        }

        const FileNode& child = scanResult.nodes[childId];
        const float angle = (static_cast<float>(index) / static_cast<float>(children.size())) * Pi * 2.0f +
            static_cast<float>(parent.depth) * 0.48f;
        const float jitter = (stableJitter(childId) - 0.5f) * settings.spacing * 0.72f;
        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec3 tangent(-radial.z, 0.0f, radial.x);

        glm::vec3 scale = nodeScale(child, scanResult, settings);
        const float lane = static_cast<float>(index % 5U) - 2.0f;
        const glm::vec3 groundPosition =
            parentGroundPosition +
            radial * (ringRadius + jitter) +
            tangent * (lane * streetOffset);

        const SceneNodeId sceneId = static_cast<SceneNodeId>(layout.nodes.size());
        layout.nodes.push_back(makeSceneNode(sceneId, child, scale, groundPosition));
        sceneIds[childId] = sceneId;

        if (child.type == FileNodeType::Directory) {
            layoutChildren(childId, groundPosition, scanResult, settings, layout, sceneIds);
        }
    }
}

void layoutCategoryDistricts(
    const FileScanResult& scanResult,
    const FileWorldSettings& settings,
    FileWorldLayout& layout)
{
    std::array<int, CategoryDistrictCount> totals{};
    std::array<int, CategoryDistrictCount> offsets{};

    for (std::size_t index = 1; index < scanResult.nodes.size(); ++index) {
        ++totals[categoryDistrict(scanResult.nodes[index].category)];
    }

    const float districtRadius = settings.spacing * settings.citySpread * 10.5f;
    const float localSpacing = settings.spacing * 1.08f;

    for (std::size_t index = 1; index < scanResult.nodes.size(); ++index) {
        const FileNode& fileNode = scanResult.nodes[index];
        const int district = categoryDistrict(fileNode.category);
        const int localIndex = offsets[district]++;
        const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(std::max(totals[district], 1))))));
        const int row = localIndex / columns;
        const int column = localIndex % columns;

        const float angle = (static_cast<float>(district) / static_cast<float>(CategoryDistrictCount)) * Pi * 2.0f;
        const glm::vec3 center(std::cos(angle) * districtRadius, 0.0f, std::sin(angle) * districtRadius);
        const float x = (static_cast<float>(column) - static_cast<float>(columns - 1) * 0.5f) * localSpacing;
        const float z = static_cast<float>(row) * localSpacing;
        const glm::vec3 tangent(-std::sin(angle), 0.0f, std::cos(angle));
        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec3 groundPosition = center + tangent * x + radial * z;

        const SceneNodeId sceneId = static_cast<SceneNodeId>(layout.nodes.size());
        layout.nodes.push_back(makeSceneNode(sceneId, fileNode, nodeScale(fileNode, scanResult, settings), groundPosition));
    }
}

void layoutSizeSkyline(
    const FileScanResult& scanResult,
    const FileWorldSettings& settings,
    FileWorldLayout& layout)
{
    std::vector<FileNodeId> ids;
    ids.reserve(scanResult.nodes.size() > 0 ? scanResult.nodes.size() - 1 : 0);

    for (std::size_t index = 1; index < scanResult.nodes.size(); ++index) {
        ids.push_back(scanResult.nodes[index].id);
    }

    std::stable_sort(ids.begin(), ids.end(), [&scanResult](FileNodeId lhs, FileNodeId rhs) {
        const FileNode& a = scanResult.nodes[lhs];
        const FileNode& b = scanResult.nodes[rhs];
        const float sizeA = nodeSortSize(a);
        const float sizeB = nodeSortSize(b);
        if (sizeA != sizeB) {
            return sizeA > sizeB;
        }
        return a.name < b.name;
    });

    const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(std::max<std::size_t>(ids.size(), 1))) * 1.42f)));
    const float laneSpacing = settings.spacing * 1.25f;
    const float rowSpacing = settings.spacing * 1.45f;
    const float startZ = settings.spacing * 4.0f;

    for (std::size_t index = 0; index < ids.size(); ++index) {
        const FileNode& fileNode = scanResult.nodes[ids[index]];
        const int row = static_cast<int>(index) / columns;
        const int column = static_cast<int>(index) % columns;
        const glm::vec3 groundPosition(
            (static_cast<float>(column) - static_cast<float>(columns - 1) * 0.5f) * laneSpacing,
            0.0f,
            startZ + static_cast<float>(row) * rowSpacing);

        const SceneNodeId sceneId = static_cast<SceneNodeId>(layout.nodes.size());
        layout.nodes.push_back(makeSceneNode(sceneId, fileNode, nodeScale(fileNode, scanResult, settings), groundPosition));
    }
}
}

const char* fileWorldLayoutModeName(FileWorldLayoutMode mode)
{
    switch (mode) {
    case FileWorldLayoutMode::FilesystemCity:
        return "Filesystem city";
    case FileWorldLayoutMode::CategoryDistricts:
        return "Category districts";
    case FileWorldLayoutMode::SizeSkyline:
    default:
        return "Size skyline";
    }
}

const char* sizeEmphasisModeName(SizeEmphasisMode mode)
{
    switch (mode) {
    case SizeEmphasisMode::Normal:
        return "Normal";
    case SizeEmphasisMode::Skyline:
    default:
        return "Skyline mode";
    }
}

bool fileCategoryAllowed(FileCategory category, const FileWorldFilters& filters)
{
    switch (category) {
    case FileCategory::Directory:
        return filters.showDirectories;
    case FileCategory::Source:
        return filters.showCode;
    case FileCategory::Image:
        return filters.showImages;
    case FileCategory::Video:
        return filters.showVideo;
    case FileCategory::Audio:
        return filters.showAudio;
    case FileCategory::Document:
        return filters.showDocuments;
    case FileCategory::Archive:
        return filters.showArchives;
    case FileCategory::Executable:
        return filters.showExecutables;
    case FileCategory::System:
    case FileCategory::Data:
    case FileCategory::Other:
    case FileCategory::Error:
    default:
        return filters.showOther;
    }
}

FileWorldLayout FileWorldBuilder::build(const FileScanResult& scanResult, const FileWorldSettings& settings) const
{
    FileWorldLayout layout;
    layout.nodes.reserve(scanResult.nodes.size());

    if (scanResult.nodes.empty()) {
        return layout;
    }

    std::unordered_map<FileNodeId, SceneNodeId> sceneIds;
    const FileNode& root = scanResult.nodes.front();
    glm::vec3 rootScale = nodeScale(root, scanResult, settings);

    SceneNode rootNode;
    rootNode.sceneId = 0;
    rootNode.sourceFileNodeId = root.id;
    rootNode.position = glm::vec3(0.0f, rootScale.y * 0.5f, 0.0f);
    rootNode.scale = rootScale;
    rootNode.color = glm::vec4(0.08f, 1.0f, 1.0f, 1.0f);
    rootNode.category = root.category;
    rootNode.label = root.name;
    rootNode.bounds = makeAabb(rootNode.position, rootNode.scale);
    rootNode.visible = true;

    layout.nodes.push_back(std::move(rootNode));
    sceneIds[root.id] = 0;

    switch (settings.layoutMode) {
    case FileWorldLayoutMode::FilesystemCity:
        layoutChildren(root.id, glm::vec3(0.0f), scanResult, settings, layout, sceneIds);
        break;
    case FileWorldLayoutMode::CategoryDistricts:
        layoutCategoryDistricts(scanResult, settings, layout);
        break;
    case FileWorldLayoutMode::SizeSkyline:
        layoutSizeSkyline(scanResult, settings, layout);
        break;
    }

    layout.visibleCount = std::count_if(layout.nodes.begin(), layout.nodes.end(), [](const SceneNode& node) {
        return node.visible;
    });

    return layout;
}
