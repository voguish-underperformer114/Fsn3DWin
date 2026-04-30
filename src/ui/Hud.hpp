#pragma once

#include "filesystem/FileScanner.hpp"
#include "renderer/InstancedCubeRenderer.hpp"
#include "scene/DemoScene.hpp"
#include "scene/FileWorldBuilder.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <string>
#include <vector>

enum class LabelMode {
    Off = 0,
    SelectedOnly,
    Important,
    LimitedAll,
};

enum class PresentationCameraMode {
    OrbitRoot = 0,
    OrbitSelected,
    FlyThrough,
};

struct OverlayLabel {
    glm::vec2 position{};
    std::string text;
    bool accent = false;
};

struct HudState {
    const char* stageName = "";
    std::string appVersion;
    double fps = 0.0;
    std::string openGlVersion;
    bool readOnly = true;
    double frameTimeMs = 0.0;
    double totalSeconds = 0.0;
    std::size_t instanceCount = 0;
    FileScanState scanState = FileScanState::Idle;
    std::size_t scanNodes = 0;
    std::size_t scanDirectories = 0;
    std::size_t scanWarnings = 0;
    double scanElapsedSeconds = 0.0;
    std::vector<OverlayLabel> overlayLabels;
    bool hoverTooltipVisible = false;
    glm::vec2 hoverTooltipPosition{};
    std::string hoverName;
    std::string hoverCategory;
    std::string hoverSize;
    std::string imagePreviewStatus;
    const std::vector<std::string>* scanLog = nullptr;
    bool presentationActive = false;
    bool presentationPaused = false;
    PresentationCameraMode presentationMode = PresentationCameraMode::OrbitRoot;
    float presentationSpeed = 1.0f;
    bool cleanHud = false;
    bool dangerActionsEnabled = false;
    bool dangerDeleteWarningAccepted = false;
    std::string screenshotStatus;
    glm::vec3 cameraPosition{};
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float cameraSpeed = 0.0f;
};

enum class HudAction {
    None,
    ResetCamera,
    Scan,
    CancelScan,
    RebuildLayout,
    ClearFilters,
    OpenSelected,
    DeleteSelected,
    Quit,
};

class Hud {
public:
    HudAction draw(
        const HudState& state,
        DemoTheme& theme,
        SceneMode& sceneMode,
        FileScanOptions& scanOptions,
        const FileScanResult* scanResult,
        FileWorldSettings& fileWorldSettings,
        const FileWorldLayout* fileWorldLayout,
        RenderMode& renderMode,
        FileWorldFilters& fileWorldFilters,
        LabelMode& labelMode,
        bool& presentationActive,
        PresentationCameraMode& presentationMode,
        bool& presentationPaused,
        float& presentationSpeed,
        bool& cleanHud,
        bool& dangerActionsEnabled,
        bool& dangerDeleteWarningAccepted,
        const SceneNode* selectedSceneNode,
        const FileNode* selectedFileNode);

private:
    void syncRootPathBuffer(const FileScanOptions& scanOptions);
    void syncFilterBuffers(const FileWorldFilters& fileWorldFilters);

    std::array<char, 1024> rootPathBuffer_{};
    std::array<char, 128> nameFilterBuffer_{};
    std::array<char, 64> extensionFilterBuffer_{};
    std::array<char, 32> deleteConfirmBuffer_{};
    std::string bufferedRootPath_;
    std::string bufferedNameFilter_;
    std::string bufferedExtensionFilter_;
    bool rootPathBufferReady_ = false;
    bool filterBuffersReady_ = false;
};

const char* labelModeName(LabelMode mode);
const char* presentationCameraModeName(PresentationCameraMode mode);
