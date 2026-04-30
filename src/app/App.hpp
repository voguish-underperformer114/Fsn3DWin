#pragma once

#include "filesystem/FileScanner.hpp"
#include "renderer/Renderer.hpp"
#include "scene/Camera.hpp"
#include "scene/DemoScene.hpp"
#include "scene/FileWorldBuilder.hpp"
#include "ui/Hud.hpp"
#include "util/Timer.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct GLFWwindow;

struct AppOptions {
    std::optional<std::filesystem::path> rootPath;
    std::optional<std::uint32_t> maxDepth;
    std::optional<std::size_t> maxNodes;
    std::optional<double> screenshotAfterSeconds;
    std::optional<double> quitAfterSeconds;
    bool autoScan = false;
    bool selectFirstImage = false;
};

class App {
public:
    explicit App(AppOptions options = {});
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int run();

private:
    void initGlfw();
    void initWindow();
    void initGl();
    void initImGui();
    void shutdownImGui();
    void pollFrame();
    void handleInput(double deltaSeconds);
    void updateMouseLook();
    void updatePresentationCamera(double deltaSeconds);
    void updatePicking(int framebufferWidth, int framebufferHeight, float aspectRatio);
    std::optional<SceneNodeId> pickSceneNode(double mouseX, double mouseY, int framebufferWidth, int framebufferHeight, float aspectRatio) const;
    const SceneNode* sceneNodeById(SceneNodeId sceneId) const;
    const FileNode* fileNodeForSceneNode(const SceneNode* sceneNode) const;
    void updateSelectedImagePreview(const SceneNode* selectedSceneNode, const FileNode* selectedFileNode);
    void clearSelectedImagePreview();
    std::optional<ImageBillboard> selectedImageBillboard(const SceneNode* selectedSceneNode) const;
    void focusSelectedNode();
    void startFocusAnimation(const glm::vec3& target, float radius);
    void updateFocusAnimation(double deltaSeconds);
    void copySelectedPathToClipboard();
    void openSelectedPath();
    void moveSelectedFileToRecycleBin();
    bool projectSceneNodeLabel(const SceneNode& sceneNode, int framebufferWidth, int framebufferHeight, float aspectRatio, glm::vec2& screenPosition) const;
    std::vector<OverlayLabel> buildOverlayLabels(int framebufferWidth, int framebufferHeight, float aspectRatio) const;
    void initializeScanDefaults();
    void performScan();
    void requestCancelScan();
    void updateScanWorker();
    bool scanInProgress() const;
    double scanElapsedSeconds() const;
    void rebuildLayout();
    void selectFirstImageNode();
    void applyFilters();
    void clearFilters();
    void appendScanLog(std::string message);
    glm::vec3 presentationTarget() const;
    float presentationTargetRadius() const;
    void requestScreenshot();
    void saveScreenshot(int framebufferWidth, int framebufferHeight);

    GLFWwindow* window_ = nullptr;
    Renderer renderer_;
    Camera camera_;
    DemoScene demoScene_;
    DemoTheme theme_ = DemoTheme::AmberTerminal;
    SceneMode sceneMode_ = SceneMode::Demo;
    RenderMode renderMode_ = RenderMode::SolidWireframe;
    LabelMode labelMode_ = LabelMode::LimitedAll;
    bool presentationActive_ = false;
    PresentationCameraMode presentationMode_ = PresentationCameraMode::OrbitRoot;
    bool presentationPaused_ = false;
    float presentationSpeed_ = 1.0f;
    bool cleanHud_ = false;
    bool autoScanRequested_ = false;
    bool selectFirstImageRequested_ = false;
    bool dangerActionsEnabled_ = false;
    bool dangerDeleteWarningAccepted_ = false;
    bool focusAnimationActive_ = false;
    glm::vec3 focusStartPosition_{};
    glm::vec3 focusEndPosition_{};
    glm::vec3 focusLookTarget_{};
    double focusAnimationElapsed_ = 0.0;
    double focusAnimationDuration_ = 0.85;
    FileScanner fileScanner_;
    FileScanOptions scanOptions_;
    FileScanResult scanResult_;
    bool hasScanResult_ = false;
    FileScanState scanState_ = FileScanState::Idle;
    std::future<FileScanResult> scanFuture_;
    std::shared_ptr<FileScanProgress> scanProgress_;
    std::shared_ptr<std::atomic_bool> scanCancelRequested_;
    double scanStartedSeconds_ = 0.0;
    double scanEndedSeconds_ = 0.0;
    FileWorldBuilder fileWorldBuilder_;
    FileWorldSettings fileWorldSettings_;
    FileWorldFilters fileWorldFilters_;
    FileWorldLayout fileWorldLayout_;
    bool hasFileWorldLayout_ = false;
    std::vector<std::string> scanLog_;
    SceneNodeId selectedSceneId_ = InvalidSceneNodeId;
    SceneNodeId hoveredSceneId_ = InvalidSceneNodeId;
    FileNodeId imagePreviewFileNodeId_ = InvalidFileNodeId;
    unsigned int imagePreviewTextureId_ = 0;
    int imagePreviewWidth_ = 0;
    int imagePreviewHeight_ = 0;
    std::string imagePreviewStatus_;
    Hud hud_;
    Timer timer_;
    std::string glVersion_;
    std::string appVersion_;
    bool mouseLookActive_ = false;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    double lastPickTime_ = -10.0;
    double lastPickMouseX_ = 0.0;
    double lastPickMouseY_ = 0.0;
    bool leftMouseWasDown_ = false;
    bool escapeWasDown_ = false;
    bool focusWasDown_ = false;
    bool copyWasDown_ = false;
    bool presentationToggleWasDown_ = false;
    bool cleanHudToggleWasDown_ = false;
    bool screenshotWasDown_ = false;
    bool screenshotRequested_ = false;
    bool scheduledScreenshotTaken_ = false;
    double scheduledScreenshotSeconds_ = -1.0;
    double scheduledQuitSeconds_ = -1.0;
    std::string screenshotStatus_;
    double lastClickTime_ = -10.0;
    SceneNodeId lastClickedSceneId_ = InvalidSceneNodeId;
    bool imguiReady_ = false;
};
