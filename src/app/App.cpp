#include "app/App.hpp"

#include "media/ImageLoader.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;
constexpr const char* WindowTitle = "Fsn3DWin";
constexpr const char* ImGuiGlslVersion = "#version 410";
#ifndef FSN3DWIN_VERSION
#define FSN3DWIN_VERSION "0.1.0"
#endif

bool keyPressedOnce(GLFWwindow* window, int key, bool& wasDown)
{
    const bool down = glfwGetKey(window, key) == GLFW_PRESS;
    const bool pressed = down && !wasDown;
    wasDown = down;
    return pressed;
}

bool rayIntersectsAabb(const glm::vec3& origin, const glm::vec3& direction, const Aabb& bounds, float& distance)
{
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis) {
        const float rayOrigin = origin[axis];
        const float rayDirection = direction[axis];
        const float minBound = bounds.min[axis];
        const float maxBound = bounds.max[axis];

        if (std::abs(rayDirection) < 0.00001f) {
            if (rayOrigin < minBound || rayOrigin > maxBound) {
                return false;
            }
            continue;
        }

        float nearDistance = (minBound - rayOrigin) / rayDirection;
        float farDistance = (maxBound - rayOrigin) / rayDirection;
        if (nearDistance > farDistance) {
            std::swap(nearDistance, farDistance);
        }

        tMin = std::max(tMin, nearDistance);
        tMax = std::min(tMax, farDistance);
        if (tMin > tMax) {
            return false;
        }
    }

    distance = tMin >= 0.0f ? tMin : tMax;
    return distance >= 0.0f;
}

std::string lowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool containsCaseInsensitive(const std::string& haystack, const std::string& needle)
{
    if (needle.empty()) {
        return true;
    }

    return lowerCopy(haystack).find(lowerCopy(needle)) != std::string::npos;
}

bool filtersEqual(const FileWorldFilters& lhs, const FileWorldFilters& rhs)
{
    return lhs.nameSubstring == rhs.nameSubstring &&
        lhs.extensionSubstring == rhs.extensionSubstring &&
        lhs.showDirectories == rhs.showDirectories &&
        lhs.showCode == rhs.showCode &&
        lhs.showImages == rhs.showImages &&
        lhs.showVideo == rhs.showVideo &&
        lhs.showAudio == rhs.showAudio &&
        lhs.showDocuments == rhs.showDocuments &&
        lhs.showArchives == rhs.showArchives &&
        lhs.showExecutables == rhs.showExecutables &&
        lhs.showOther == rhs.showOther;
}

glm::vec3 cameraForwardFromAngles(float yawDegrees, float pitchDegrees)
{
    const float yawRadians = glm::radians(yawDegrees);
    const float pitchRadians = glm::radians(pitchDegrees);
    return glm::normalize(glm::vec3(
        std::cos(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::sin(yawRadians) * std::cos(pitchRadians)));
}

float smoothstep01(float value)
{
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

std::string formatBytesForOverlay(const FileNode& fileNode)
{
    if (!fileNode.sizeKnown) {
        return "size: unknown";
    }

    constexpr double KiB = 1024.0;
    constexpr double MiB = KiB * 1024.0;
    constexpr double GiB = MiB * 1024.0;
    const double bytes = static_cast<double>(fileNode.sizeBytes);
    std::ostringstream stream;

    if (bytes >= GiB) {
        stream << "size: " << (bytes / GiB) << " GiB";
    } else if (bytes >= MiB) {
        stream << "size: " << (bytes / MiB) << " MiB";
    } else if (bytes >= KiB) {
        stream << "size: " << (bytes / KiB) << " KiB";
    } else {
        stream << "size: " << fileNode.sizeBytes << " B";
    }

    return stream.str();
}
}

App::App(AppOptions options)
    : appVersion_(FSN3DWIN_VERSION)
{
    initializeScanDefaults();
    if (options.rootPath.has_value()) {
        scanOptions_.rootPath = *options.rootPath;
    }
    if (options.maxDepth.has_value()) {
        scanOptions_.maxDepth = *options.maxDepth;
    }
    if (options.maxNodes.has_value()) {
        scanOptions_.maxNodes = *options.maxNodes;
    }
    autoScanRequested_ = options.autoScan;
    selectFirstImageRequested_ = options.selectFirstImage;
    if (options.screenshotAfterSeconds.has_value()) {
        scheduledScreenshotSeconds_ = std::max(0.0, *options.screenshotAfterSeconds);
    }
    if (options.quitAfterSeconds.has_value()) {
        scheduledQuitSeconds_ = std::max(0.0, *options.quitAfterSeconds);
    }

    initGlfw();
    initWindow();
    initGl();
    initImGui();
}

App::~App()
{
    if (scanFuture_.valid()) {
        if (scanCancelRequested_ != nullptr) {
            scanCancelRequested_->store(true, std::memory_order_relaxed);
        }
        scanFuture_.wait();
    }

    shutdownImGui();
    clearSelectedImagePreview();
    renderer_.shutdown();

    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

int App::run()
{
    while (!glfwWindowShouldClose(window_)) {
        pollFrame();
    }

    return 0;
}

void App::initGlfw()
{
    glfwSetErrorCallback([](int, const char* description) {
        std::cerr << "GLFW error: " << (description != nullptr ? description : "Unknown") << '\n';
    });

    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void App::initWindow()
{
    window_ = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle, nullptr, nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
}

void App::initGl()
{
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        throw std::runtime_error("Failed to load OpenGL functions with GLAD");
    }

    const auto* version = glGetString(GL_VERSION);
    glVersion_ = version != nullptr ? reinterpret_cast<const char*>(version) : "Unknown";

    renderer_.initialize();
}

void App::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ScrollbarSize = 12.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.WindowPadding = ImVec2(12.0f, 10.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.015f, 0.035f, 0.060f, 0.91f);
    colors[ImGuiCol_Border] = ImVec4(0.22f, 0.88f, 0.95f, 0.38f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.035f, 0.085f, 0.120f, 0.95f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.055f, 0.170f, 0.210f, 0.95f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.075f, 0.230f, 0.270f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.012f, 0.040f, 0.065f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.025f, 0.120f, 0.155f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 1.0f, 0.90f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.95f, 0.90f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.86f, 0.32f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.035f, 0.150f, 0.185f, 0.95f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.070f, 0.250f, 0.300f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.50f, 0.52f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.035f, 0.180f, 0.205f, 0.82f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.060f, 0.280f, 0.320f, 0.95f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.42f, 0.44f, 1.0f);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        throw std::runtime_error("Failed to initialize ImGui GLFW backend");
    }

    if (!ImGui_ImplOpenGL3_Init(ImGuiGlslVersion)) {
        throw std::runtime_error("Failed to initialize ImGui OpenGL backend");
    }

    imguiReady_ = true;
}

void App::shutdownImGui()
{
    if (!imguiReady_) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    imguiReady_ = false;
}

void App::pollFrame()
{
    glfwPollEvents();
    timer_.tick();
    updateScanWorker();
    if (autoScanRequested_ && !scanInProgress() && !hasScanResult_) {
        autoScanRequested_ = false;
        performScan();
    }
    handleInput(timer_.deltaSeconds());
    updateFocusAnimation(timer_.deltaSeconds());
    updatePresentationCamera(timer_.deltaSeconds());

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    const float aspectRatio = static_cast<float>(std::max(framebufferWidth, 1)) /
        static_cast<float>(std::max(framebufferHeight, 1));
    const DemoPalette palette = DemoScene::palette(theme_);
    updatePicking(framebufferWidth, framebufferHeight, aspectRatio);

    const SceneNode* selectedSceneNode = sceneNodeById(selectedSceneId_);
    const FileNode* selectedFileNode = fileNodeForSceneNode(selectedSceneNode);
    updateSelectedImagePreview(selectedSceneNode, selectedFileNode);
    const std::optional<ImageBillboard> imageBillboard = selectedImageBillboard(selectedSceneNode);

    renderer_.beginFrame(framebufferWidth, framebufferHeight, palette.background);
    renderer_.renderScene(
        camera_,
        aspectRatio,
        static_cast<float>(timer_.totalSeconds()),
        demoScene_,
        theme_,
        sceneMode_,
        hasFileWorldLayout_ ? &fileWorldLayout_.nodes : nullptr,
        renderMode_,
        selectedSceneId_,
        hoveredSceneId_,
        scanInProgress(),
        imageBillboard.has_value() ? &*imageBillboard : nullptr);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseWheel != 0.0f && !io.WantCaptureMouse) {
        camera_.changeMovementSpeed(io.MouseWheel);
    }

    const SceneNode* hoveredSceneNode = sceneNodeById(hoveredSceneId_);
    const FileNode* hoveredFileNode = fileNodeForSceneNode(hoveredSceneNode);

    glm::vec2 hoverTooltipPosition{};
    bool hoverTooltipVisible = false;
    if (hoveredSceneNode != nullptr) {
        hoverTooltipVisible = projectSceneNodeLabel(*hoveredSceneNode, framebufferWidth, framebufferHeight, aspectRatio, hoverTooltipPosition);
    }

    const HudState state{
        .stageName = "Public preview build",
        .appVersion = appVersion_,
        .fps = timer_.fps(),
        .openGlVersion = glVersion_,
        .readOnly = true,
        .frameTimeMs = timer_.deltaSeconds() * 1000.0,
        .totalSeconds = timer_.totalSeconds(),
        .instanceCount = renderer_.lastInstanceCount(),
        .scanState = scanState_,
        .scanNodes = scanProgress_ != nullptr ? scanProgress_->nodesScanned.load(std::memory_order_relaxed) : scanResult_.counts.nodes,
        .scanDirectories = scanProgress_ != nullptr ? scanProgress_->directoriesScanned.load(std::memory_order_relaxed) : scanResult_.counts.directories,
        .scanWarnings = scanProgress_ != nullptr ? scanProgress_->warnings.load(std::memory_order_relaxed) : scanResult_.warnings.size(),
        .scanElapsedSeconds = scanElapsedSeconds(),
        .overlayLabels = buildOverlayLabels(framebufferWidth, framebufferHeight, aspectRatio),
        .hoverTooltipVisible = hoverTooltipVisible && hoveredFileNode != nullptr,
        .hoverTooltipPosition = hoverTooltipPosition,
        .hoverName = hoveredFileNode != nullptr ? hoveredFileNode->name : std::string(),
        .hoverCategory = hoveredFileNode != nullptr ? std::string("category: ") + fileCategoryName(hoveredFileNode->category) : std::string(),
        .hoverSize = hoveredFileNode != nullptr ? formatBytesForOverlay(*hoveredFileNode) : std::string(),
        .imagePreviewStatus = imagePreviewStatus_,
        .scanLog = &scanLog_,
        .presentationActive = presentationActive_,
        .presentationPaused = presentationPaused_,
        .presentationMode = presentationMode_,
        .presentationSpeed = presentationSpeed_,
        .cleanHud = cleanHud_,
        .dangerActionsEnabled = dangerActionsEnabled_,
        .dangerDeleteWarningAccepted = dangerDeleteWarningAccepted_,
        .screenshotStatus = screenshotStatus_,
        .cameraPosition = camera_.position(),
        .cameraYaw = camera_.yaw(),
        .cameraPitch = camera_.pitch(),
        .cameraSpeed = camera_.movementSpeed(),
    };

    const FileWorldFilters filtersBeforeHud = fileWorldFilters_;
    const HudAction action = hud_.draw(
        state,
        theme_,
        sceneMode_,
        scanOptions_,
        hasScanResult_ ? &scanResult_ : nullptr,
        fileWorldSettings_,
        hasFileWorldLayout_ ? &fileWorldLayout_ : nullptr,
        renderMode_,
        fileWorldFilters_,
        labelMode_,
        presentationActive_,
        presentationMode_,
        presentationPaused_,
        presentationSpeed_,
        cleanHud_,
        dangerActionsEnabled_,
        dangerDeleteWarningAccepted_,
        selectedSceneNode,
        selectedFileNode);

    bool filtersAlreadyApplied = false;

    if (action == HudAction::Quit) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    } else if (action == HudAction::ResetCamera) {
        focusAnimationActive_ = false;
        camera_.reset();
    } else if (action == HudAction::Scan) {
        performScan();
    } else if (action == HudAction::CancelScan) {
        requestCancelScan();
    } else if (action == HudAction::RebuildLayout) {
        rebuildLayout();
        filtersAlreadyApplied = true;
    } else if (action == HudAction::ClearFilters) {
        clearFilters();
        filtersAlreadyApplied = true;
    } else if (action == HudAction::OpenSelected) {
        openSelectedPath();
    } else if (action == HudAction::DeleteSelected) {
        moveSelectedFileToRecycleBin();
    }

    if (!filtersAlreadyApplied && !filtersEqual(filtersBeforeHud, fileWorldFilters_)) {
        applyFilters();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (!scheduledScreenshotTaken_ &&
        scheduledScreenshotSeconds_ >= 0.0 &&
        timer_.totalSeconds() >= scheduledScreenshotSeconds_) {
        requestScreenshot();
        scheduledScreenshotTaken_ = true;
    }

    if (screenshotRequested_) {
        saveScreenshot(framebufferWidth, framebufferHeight);
        screenshotRequested_ = false;
    }

    if (scheduledQuitSeconds_ >= 0.0 && timer_.totalSeconds() >= scheduledQuitSeconds_) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }

    glfwSwapBuffers(window_);
}

void App::handleInput(double deltaSeconds)
{
    const ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureKeyboard) {
        if (keyPressedOnce(window_, GLFW_KEY_P, presentationToggleWasDown_)) {
            presentationActive_ = !presentationActive_;
            focusAnimationActive_ = false;
            appendScanLog(presentationActive_ ? "presentation mode engaged" : "presentation mode disengaged");
        }

        if (keyPressedOnce(window_, GLFW_KEY_H, cleanHudToggleWasDown_)) {
            cleanHud_ = !cleanHud_;
        }

        if (keyPressedOnce(window_, GLFW_KEY_F12, screenshotWasDown_)) {
            requestScreenshot();
        }
    }

    if (presentationActive_ && !presentationPaused_) {
        return;
    }

    float forward = 0.0f;
    float right = 0.0f;
    float up = 0.0f;

    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
            forward += 1.0f;
        }

        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
            forward -= 1.0f;
        }

        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
            right += 1.0f;
        }

        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
            right -= 1.0f;
        }

        if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
            up += 1.0f;
        }

        if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS) {
            up -= 1.0f;
        }
    }

    camera_.move(forward, right, up, static_cast<float>(deltaSeconds));
    if (std::abs(forward) > 0.0f || std::abs(right) > 0.0f || std::abs(up) > 0.0f) {
        focusAnimationActive_ = false;
    }
    updateMouseLook();
}

void App::updatePresentationCamera(double deltaSeconds)
{
    if (!presentationActive_ || presentationPaused_) {
        return;
    }

    const float t = static_cast<float>(timer_.totalSeconds() * std::max(0.05f, presentationSpeed_));
    const glm::vec3 target = presentationTarget();
    const float radius = presentationTargetRadius();

    if (presentationMode_ == PresentationCameraMode::OrbitRoot ||
        presentationMode_ == PresentationCameraMode::OrbitSelected) {
        const float distance = std::clamp(radius * 3.8f + 12.0f, 12.0f, 90.0f);
        const float angle = t * 0.32f;
        const float bob = std::sin(t * 0.55f) * std::min(radius * 0.18f, 2.5f);
        const glm::vec3 position(
            target.x + std::cos(angle) * distance,
            target.y + radius * 0.52f + 7.0f + bob,
            target.z + std::sin(angle) * distance);
        camera_.setPosition(position);
        camera_.lookAt(target + glm::vec3(0.0f, std::min(radius * 0.2f, 3.0f), 0.0f));
        return;
    }

    const float lane = std::sin(t * 0.18f) * 11.0f;
    const float sweep = std::fmod(t * 5.4f, 70.0f) - 35.0f;
    const glm::vec3 position(lane, 5.5f + std::sin(t * 0.44f) * 2.2f, sweep);
    const glm::vec3 lookTarget(
        std::sin(t * 0.21f) * 8.0f,
        4.0f + std::cos(t * 0.37f) * 1.6f,
        sweep + 18.0f);
    camera_.setPosition(position);
    camera_.lookAt(lookTarget);
    (void)deltaSeconds;
}

void App::updateMouseLook()
{
    const bool rightMouseDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const ImGuiIO& io = ImGui::GetIO();
    const bool canLook = rightMouseDown && (!io.WantCaptureMouse || mouseLookActive_);

    if (!canLook) {
        if (mouseLookActive_) {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        mouseLookActive_ = false;
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);

    if (!mouseLookActive_) {
        mouseLookActive_ = true;
        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        return;
    }

    camera_.look(static_cast<float>(mouseX - lastMouseX_), static_cast<float>(mouseY - lastMouseY_));
    focusAnimationActive_ = false;
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
}

void App::updatePicking(int framebufferWidth, int framebufferHeight, float aspectRatio)
{
    const ImGuiIO& io = ImGui::GetIO();
    const bool canPick = sceneMode_ == SceneMode::RealScan &&
        hasFileWorldLayout_ &&
        !io.WantCaptureMouse &&
        !mouseLookActive_ &&
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS;

    if (keyPressedOnce(window_, GLFW_KEY_ESCAPE, escapeWasDown_)) {
        selectedSceneId_ = InvalidSceneNodeId;
        dangerDeleteWarningAccepted_ = false;
    }

    if (keyPressedOnce(window_, GLFW_KEY_F, focusWasDown_)) {
        focusSelectedNode();
    }

    const bool copyDown = (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) &&
        glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    if (copyDown && !copyWasDown_) {
        copySelectedPathToClipboard();
    }
    copyWasDown_ = copyDown;

    if (!canPick) {
        hoveredSceneId_ = InvalidSceneNodeId;
        leftMouseWasDown_ = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        lastPickTime_ = -10.0;
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);

    const bool leftMouseDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool clickStarted = leftMouseDown && !leftMouseWasDown_;
    const double now = timer_.totalSeconds();
    const bool mouseMoved =
        std::abs(mouseX - lastPickMouseX_) > 0.5 ||
        std::abs(mouseY - lastPickMouseY_) > 0.5;
    const bool pickDue = (now - lastPickTime_) >= (1.0 / 30.0);

    if (clickStarted || mouseMoved || pickDue) {
        const std::optional<SceneNodeId> hovered = pickSceneNode(mouseX, mouseY, framebufferWidth, framebufferHeight, aspectRatio);
        hoveredSceneId_ = hovered.value_or(InvalidSceneNodeId);
        lastPickMouseX_ = mouseX;
        lastPickMouseY_ = mouseY;
        lastPickTime_ = now;
    }

    if (clickStarted) {
        const SceneNodeId clickedSceneId = hoveredSceneId_;
        if (selectedSceneId_ != clickedSceneId) {
            dangerDeleteWarningAccepted_ = false;
        }
        selectedSceneId_ = clickedSceneId;

        const bool doubleClick = clickedSceneId != InvalidSceneNodeId &&
            clickedSceneId == lastClickedSceneId_ &&
            (timer_.totalSeconds() - lastClickTime_) <= 0.35;

        if (doubleClick) {
            const SceneNode* sceneNode = sceneNodeById(clickedSceneId);
            const FileNode* fileNode = fileNodeForSceneNode(sceneNode);
            if (fileNode != nullptr && fileNode->type == FileNodeType::Directory) {
                focusSelectedNode();
            }
        }

        lastClickedSceneId_ = clickedSceneId;
        lastClickTime_ = timer_.totalSeconds();
    }

    leftMouseWasDown_ = leftMouseDown;
}

std::optional<SceneNodeId> App::pickSceneNode(double mouseX, double mouseY, int framebufferWidth, int framebufferHeight, float aspectRatio) const
{
    if (!hasFileWorldLayout_ || framebufferWidth <= 0 || framebufferHeight <= 0) {
        return std::nullopt;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);
    windowWidth = std::max(windowWidth, 1);
    windowHeight = std::max(windowHeight, 1);

    const float ndcX = static_cast<float>((mouseX / static_cast<double>(windowWidth)) * 2.0 - 1.0);
    const float ndcY = static_cast<float>(1.0 - (mouseY / static_cast<double>(windowHeight)) * 2.0);

    const glm::mat4 inverseViewProjection = glm::inverse(camera_.projectionMatrix(aspectRatio) * camera_.viewMatrix());
    glm::vec4 nearPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    const glm::vec3 rayOrigin = camera_.position();
    const glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint - nearPoint));

    SceneNodeId nearestSceneId = InvalidSceneNodeId;
    float nearestDistance = std::numeric_limits<float>::max();

    for (const SceneNode& node : fileWorldLayout_.nodes) {
        if (!node.visible) {
            continue;
        }

        float distance = 0.0f;
        if (rayIntersectsAabb(rayOrigin, rayDirection, node.bounds, distance) && distance < nearestDistance) {
            nearestDistance = distance;
            nearestSceneId = node.sceneId;
        }
    }

    if (nearestSceneId == InvalidSceneNodeId) {
        return std::nullopt;
    }

    return nearestSceneId;
}

const SceneNode* App::sceneNodeById(SceneNodeId sceneId) const
{
    if (!hasFileWorldLayout_ || sceneId == InvalidSceneNodeId) {
        return nullptr;
    }

    const std::size_t index = static_cast<std::size_t>(sceneId);
    if (index < fileWorldLayout_.nodes.size() && fileWorldLayout_.nodes[index].sceneId == sceneId) {
        return &fileWorldLayout_.nodes[index];
    }

    return nullptr;
}

const FileNode* App::fileNodeForSceneNode(const SceneNode* sceneNode) const
{
    if (sceneNode == nullptr || !hasScanResult_ || sceneNode->sourceFileNodeId >= scanResult_.nodes.size()) {
        return nullptr;
    }

    return &scanResult_.nodes[sceneNode->sourceFileNodeId];
}

void App::updateSelectedImagePreview(const SceneNode* selectedSceneNode, const FileNode* selectedFileNode)
{
    if (selectedSceneNode == nullptr || selectedFileNode == nullptr) {
        clearSelectedImagePreview();
        return;
    }

    if (selectedFileNode->type != FileNodeType::File || selectedFileNode->category != FileCategory::Image) {
        clearSelectedImagePreview();
        imagePreviewStatus_ = "selected item is not an image file";
        return;
    }

    if (imagePreviewFileNodeId_ == selectedFileNode->id) {
        return;
    }

    clearSelectedImagePreview();
    imagePreviewFileNodeId_ = selectedFileNode->id;
    imagePreviewStatus_ = "loading " + selectedFileNode->name;

    const LoadedImage image = loadImageFileRgba(selectedFileNode->fullPath);
    if (!image.ok) {
        imagePreviewStatus_ = "preview failed: " + image.error;
        return;
    }

    glGenTextures(1, &imagePreviewTextureId_);
    glBindTexture(GL_TEXTURE_2D, imagePreviewTextureId_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        image.width,
        image.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    imagePreviewWidth_ = image.width;
    imagePreviewHeight_ = image.height;
    imagePreviewStatus_ = "showing " + selectedFileNode->name + " (" +
        std::to_string(imagePreviewWidth_) + "x" + std::to_string(imagePreviewHeight_) + ")";
}

void App::clearSelectedImagePreview()
{
    if (imagePreviewTextureId_ != 0) {
        glDeleteTextures(1, &imagePreviewTextureId_);
        imagePreviewTextureId_ = 0;
    }

    imagePreviewFileNodeId_ = InvalidFileNodeId;
    imagePreviewWidth_ = 0;
    imagePreviewHeight_ = 0;
    imagePreviewStatus_.clear();
}

std::optional<ImageBillboard> App::selectedImageBillboard(const SceneNode* selectedSceneNode) const
{
    if (selectedSceneNode == nullptr || imagePreviewTextureId_ == 0 || imagePreviewWidth_ <= 0 || imagePreviewHeight_ <= 0) {
        return std::nullopt;
    }

    const float aspect = static_cast<float>(imagePreviewWidth_) / static_cast<float>(std::max(imagePreviewHeight_, 1));
    float width = 5.2f;
    float height = width / std::max(aspect, 0.05f);
    if (height > 4.2f) {
        height = 4.2f;
        width = height * aspect;
    }

    width = std::clamp(width, 1.6f, 6.4f);
    height = std::clamp(height, 1.2f, 4.4f);

    ImageBillboard billboard;
    billboard.textureId = imagePreviewTextureId_;
    billboard.width = width;
    billboard.height = height;
    billboard.position = selectedSceneNode->position + glm::vec3(
        0.0f,
        selectedSceneNode->scale.y * 0.58f + height * 0.5f + 1.25f,
        0.0f);
    return billboard;
}

void App::focusSelectedNode()
{
    const SceneNode* node = sceneNodeById(selectedSceneId_);
    if (node == nullptr) {
        return;
    }

    const float radius = std::max({node->scale.x, node->scale.y, node->scale.z});
    presentationActive_ = false;
    startFocusAnimation(node->position, radius);
}

void App::startFocusAnimation(const glm::vec3& target, float radius)
{
    const float distance = std::clamp(radius * 4.0f + 6.5f, 7.5f, 96.0f);
    glm::vec3 viewDirection = cameraForwardFromAngles(camera_.yaw(), camera_.pitch());
    if (glm::length(viewDirection) < 0.001f) {
        viewDirection = glm::normalize(glm::vec3(-1.0f, -0.35f, -1.0f));
    }

    focusStartPosition_ = camera_.position();
    focusEndPosition_ = target - viewDirection * distance + glm::vec3(0.0f, std::min(radius * 0.35f, 4.0f), 0.0f);
    focusLookTarget_ = target;
    focusAnimationElapsed_ = 0.0;
    focusAnimationDuration_ = 0.85;
    focusAnimationActive_ = true;
}

void App::updateFocusAnimation(double deltaSeconds)
{
    if (!focusAnimationActive_ || (presentationActive_ && !presentationPaused_)) {
        return;
    }

    focusAnimationElapsed_ += std::max(0.0, deltaSeconds);
    const float amount = smoothstep01(static_cast<float>(focusAnimationElapsed_ / focusAnimationDuration_));
    const glm::vec3 position = focusStartPosition_ + (focusEndPosition_ - focusStartPosition_) * amount;
    camera_.setPosition(position);
    camera_.lookAt(focusLookTarget_);

    if (focusAnimationElapsed_ >= focusAnimationDuration_) {
        focusAnimationActive_ = false;
    }
}

void App::copySelectedPathToClipboard()
{
    const SceneNode* sceneNode = sceneNodeById(selectedSceneId_);
    const FileNode* fileNode = fileNodeForSceneNode(sceneNode);
    if (fileNode == nullptr) {
        return;
    }

    const std::string path = fileNode->fullPath.string();
    glfwSetClipboardString(window_, path.c_str());
}

void App::openSelectedPath()
{
    if (!dangerActionsEnabled_) {
        appendScanLog("open blocked: danger actions are disabled");
        return;
    }

    const SceneNode* sceneNode = sceneNodeById(selectedSceneId_);
    const FileNode* fileNode = fileNodeForSceneNode(sceneNode);
    if (fileNode == nullptr) {
        appendScanLog("open blocked: no selected item");
        return;
    }

#ifdef _WIN32
    const HINSTANCE result = ShellExecuteW(
        nullptr,
        L"open",
        fileNode->fullPath.wstring().c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(result) <= 32) {
        appendScanLog("open failed: Windows rejected the selected path");
        return;
    }

    appendScanLog("opened with Windows: " + fileNode->name);
#else
    appendScanLog("open is only implemented on Windows");
#endif
}

void App::moveSelectedFileToRecycleBin()
{
    if (!dangerActionsEnabled_ || !dangerDeleteWarningAccepted_) {
        appendScanLog("delete blocked: warnings are not accepted");
        return;
    }

    const SceneNode* sceneNode = sceneNodeById(selectedSceneId_);
    const FileNode* fileNode = fileNodeForSceneNode(sceneNode);
    if (fileNode == nullptr) {
        appendScanLog("delete blocked: no selected item");
        return;
    }

    if (fileNode->type != FileNodeType::File) {
        appendScanLog("delete blocked: only files can be moved to Recycle Bin");
        return;
    }

#ifdef _WIN32
    std::wstring from = fileNode->fullPath.wstring();
    from.push_back(L'\0');
    from.push_back(L'\0');

    SHFILEOPSTRUCTW operation{};
    operation.wFunc = FO_DELETE;
    operation.pFrom = from.c_str();
    operation.fFlags = FOF_ALLOWUNDO | FOF_WANTNUKEWARNING;

    const int result = SHFileOperationW(&operation);
    if (result != 0 || operation.fAnyOperationsAborted) {
        appendScanLog("delete canceled or failed");
        return;
    }

    appendScanLog("moved to Recycle Bin: " + fileNode->name);
    dangerDeleteWarningAccepted_ = false;
    selectedSceneId_ = InvalidSceneNodeId;
    hoveredSceneId_ = InvalidSceneNodeId;
    clearSelectedImagePreview();
    performScan();
#else
    appendScanLog("delete is only implemented on Windows");
#endif
}

bool App::projectSceneNodeLabel(const SceneNode& sceneNode, int framebufferWidth, int framebufferHeight, float aspectRatio, glm::vec2& screenPosition) const
{
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return false;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);
    windowWidth = std::max(windowWidth, 1);
    windowHeight = std::max(windowHeight, 1);

    const glm::vec3 labelWorldPosition = sceneNode.position + glm::vec3(0.0f, sceneNode.scale.y * 0.65f + 0.35f, 0.0f);
    const glm::vec4 clip = camera_.projectionMatrix(aspectRatio) * camera_.viewMatrix() * glm::vec4(labelWorldPosition, 1.0f);
    if (clip.w <= 0.0f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) {
        return false;
    }

    screenPosition = glm::vec2(
        (ndc.x * 0.5f + 0.5f) * static_cast<float>(windowWidth),
        (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(windowHeight));
    return true;
}

std::vector<OverlayLabel> App::buildOverlayLabels(int framebufferWidth, int framebufferHeight, float aspectRatio) const
{
    std::vector<OverlayLabel> labels;
    if (labelMode_ == LabelMode::Off || !hasFileWorldLayout_) {
        return labels;
    }

    auto addLabel = [&](const SceneNode& node, bool accent) {
        if (!node.visible) {
            return;
        }

        glm::vec2 screenPosition{};
        if (!projectSceneNodeLabel(node, framebufferWidth, framebufferHeight, aspectRatio, screenPosition)) {
            return;
        }

        labels.push_back({
            .position = screenPosition,
            .text = node.label,
            .accent = accent,
        });
    };

    const SceneNode* selected = sceneNodeById(selectedSceneId_);
    if (selected != nullptr) {
        addLabel(*selected, true);
    }

    if (labelMode_ == LabelMode::SelectedOnly) {
        return labels;
    }

    const SceneNode* hovered = sceneNodeById(hoveredSceneId_);
    if (hovered != nullptr && hovered != selected) {
        addLabel(*hovered, true);
    }

    if (labelMode_ == LabelMode::Important || labelMode_ == LabelMode::LimitedAll) {
        if (!fileWorldLayout_.nodes.empty()) {
            addLabel(fileWorldLayout_.nodes.front(), true);
        }

        std::vector<const SceneNode*> directoryNodes;
        directoryNodes.reserve(12);
        for (const SceneNode& node : fileWorldLayout_.nodes) {
            if (!node.visible || node.category != FileCategory::Directory || node.sceneId == 0) {
                continue;
            }

            directoryNodes.push_back(&node);
        }

        std::stable_sort(directoryNodes.begin(), directoryNodes.end(), [](const SceneNode* a, const SceneNode* b) {
            return a->scale.y > b->scale.y;
        });

        const std::size_t importantCount = std::min<std::size_t>(directoryNodes.size(), 8);
        for (std::size_t index = 0; index < importantCount; ++index) {
            addLabel(*directoryNodes[index], false);
        }
    }

    if (labelMode_ == LabelMode::LimitedAll) {
        constexpr std::size_t MaxLabels = 60;
        for (const SceneNode& node : fileWorldLayout_.nodes) {
            if (labels.size() >= MaxLabels) {
                break;
            }

            if (!node.visible || node.category == FileCategory::Directory) {
                continue;
            }

            addLabel(node, false);
        }
    }

    return labels;
}

glm::vec3 App::presentationTarget() const
{
    if (presentationMode_ == PresentationCameraMode::OrbitSelected) {
        const SceneNode* selected = sceneNodeById(selectedSceneId_);
        if (selected != nullptr) {
            return selected->position;
        }
    }

    if (hasFileWorldLayout_ && !fileWorldLayout_.nodes.empty()) {
        return fileWorldLayout_.nodes.front().position;
    }

    return glm::vec3(0.0f, 4.5f, 0.0f);
}

float App::presentationTargetRadius() const
{
    const SceneNode* node = nullptr;
    if (presentationMode_ == PresentationCameraMode::OrbitSelected) {
        node = sceneNodeById(selectedSceneId_);
    }

    if (node == nullptr && hasFileWorldLayout_ && !fileWorldLayout_.nodes.empty()) {
        node = &fileWorldLayout_.nodes.front();
    }

    if (node == nullptr) {
        return 8.0f;
    }

    return std::max({node->scale.x, node->scale.y, node->scale.z});
}

void App::requestScreenshot()
{
    screenshotRequested_ = true;
    screenshotStatus_ = "screenshot queued";
}

void App::saveScreenshot(int framebufferWidth, int framebufferHeight)
{
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        screenshotStatus_ = "screenshot failed: empty framebuffer";
        return;
    }

    try {
        const std::filesystem::path screenshotDir = std::filesystem::current_path() / "screenshots";
        std::filesystem::create_directories(screenshotDir);

        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &nowTime);
#else
        localtime_r(&nowTime, &localTime);
#endif

        std::ostringstream filename;
        filename << "Fsn3DWin_"
                 << std::put_time(&localTime, "%Y%m%d_%H%M%S")
                 << ".bmp";
        const std::filesystem::path path = screenshotDir / filename.str();

        const int width = framebufferWidth;
        const int height = framebufferHeight;
        const int rowBytes = width * 3;
        const int paddedRowBytes = (rowBytes + 3) & ~3;
        const int pixelDataSize = paddedRowBytes * height;
        const int fileSize = 54 + pixelDataSize;

        std::vector<unsigned char> pixels(static_cast<std::size_t>(rowBytes) * static_cast<std::size_t>(height));
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

        std::ofstream output(path, std::ios::binary);
        if (!output) {
            screenshotStatus_ = "screenshot failed: cannot write file";
            return;
        }

        auto writeU16 = [&output](std::uint16_t value) {
            output.put(static_cast<char>(value & 0xFFU));
            output.put(static_cast<char>((value >> 8U) & 0xFFU));
        };
        auto writeU32 = [&output](std::uint32_t value) {
            output.put(static_cast<char>(value & 0xFFU));
            output.put(static_cast<char>((value >> 8U) & 0xFFU));
            output.put(static_cast<char>((value >> 16U) & 0xFFU));
            output.put(static_cast<char>((value >> 24U) & 0xFFU));
        };

        output.put('B');
        output.put('M');
        writeU32(static_cast<std::uint32_t>(fileSize));
        writeU16(0);
        writeU16(0);
        writeU32(54);
        writeU32(40);
        writeU32(static_cast<std::uint32_t>(width));
        writeU32(static_cast<std::uint32_t>(height));
        writeU16(1);
        writeU16(24);
        writeU32(0);
        writeU32(static_cast<std::uint32_t>(pixelDataSize));
        writeU32(2835);
        writeU32(2835);
        writeU32(0);
        writeU32(0);

        const std::array<unsigned char, 3> padding{0, 0, 0};
        for (int row = 0; row < height; ++row) {
            const auto* rowStart = pixels.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(rowBytes);
            output.write(reinterpret_cast<const char*>(rowStart), rowBytes);
            output.write(reinterpret_cast<const char*>(padding.data()), paddedRowBytes - rowBytes);
        }

        screenshotStatus_ = "screenshot saved: screenshots/" + filename.str();
        appendScanLog(screenshotStatus_);
    } catch (const std::exception& error) {
        screenshotStatus_ = std::string("screenshot failed: ") + error.what();
    } catch (...) {
        screenshotStatus_ = "screenshot failed";
    }
}

void App::initializeScanDefaults()
{
    std::error_code error;
    scanOptions_.rootPath = std::filesystem::current_path(error);
    if (error) {
        scanOptions_.rootPath = ".";
    }

    scanOptions_.maxDepth = 4;
    scanOptions_.maxNodes = 1500;
    scanOptions_.includeHidden = false;
    scanOptions_.followSymlinks = false;
}

void App::performScan()
{
    if (scanInProgress()) {
        return;
    }

    scanProgress_ = std::make_shared<FileScanProgress>();
    scanCancelRequested_ = std::make_shared<std::atomic_bool>(false);
    scanStartedSeconds_ = timer_.totalSeconds();
    scanEndedSeconds_ = scanStartedSeconds_;
    scanState_ = FileScanState::Scanning;
    selectedSceneId_ = InvalidSceneNodeId;
    hoveredSceneId_ = InvalidSceneNodeId;
    appendScanLog("scan started: " + scanOptions_.rootPath.string());

    const FileScanOptions options = scanOptions_;
    const std::shared_ptr<FileScanProgress> progress = scanProgress_;
    const std::shared_ptr<std::atomic_bool> cancelFlag = scanCancelRequested_;
    scanFuture_ = std::async(std::launch::async, [options, progress, cancelFlag]() {
        FileScanner scanner;
        return scanner.scan(options, progress.get(), cancelFlag.get());
    });
}

void App::requestCancelScan()
{
    if (!scanInProgress() || scanCancelRequested_ == nullptr) {
        return;
    }

    scanCancelRequested_->store(true, std::memory_order_relaxed);
    scanState_ = FileScanState::CancelRequested;
    appendScanLog("cancel requested");
}

void App::updateScanWorker()
{
    if (!scanFuture_.valid()) {
        return;
    }

    const std::future_status status = scanFuture_.wait_for(std::chrono::seconds(0));
    if (status != std::future_status::ready) {
        return;
    }

    scanEndedSeconds_ = timer_.totalSeconds();

    try {
        scanResult_ = scanFuture_.get();
        hasScanResult_ = true;
        const bool canceled = scanCancelRequested_ != nullptr &&
            scanCancelRequested_->load(std::memory_order_relaxed);
        scanState_ = scanResult_.errors.empty() ? FileScanState::Complete : FileScanState::Failed;
        appendScanLog(
            (canceled ? "scan canceled: " : "scan complete: ") +
            std::to_string(scanResult_.counts.nodes) + " nodes");
        rebuildLayout();
        if (selectFirstImageRequested_) {
            selectFirstImageNode();
            selectFirstImageRequested_ = false;
        }
    } catch (const std::exception& error) {
        scanResult_ = FileScanResult();
        scanResult_.errors.push_back(std::string("Worker failed: ") + error.what());
        scanState_ = FileScanState::Failed;
        hasScanResult_ = true;
        appendScanLog("scan failed");
    } catch (...) {
        scanResult_ = FileScanResult();
        scanResult_.errors.push_back("Worker failed with an unknown error.");
        scanState_ = FileScanState::Failed;
        hasScanResult_ = true;
        appendScanLog("scan failed");
    }
}

bool App::scanInProgress() const
{
    return scanState_ == FileScanState::Scanning || scanState_ == FileScanState::CancelRequested;
}

double App::scanElapsedSeconds() const
{
    if (scanInProgress()) {
        return std::max(0.0, timer_.totalSeconds() - scanStartedSeconds_);
    }

    return std::max(0.0, scanEndedSeconds_ - scanStartedSeconds_);
}

void App::rebuildLayout()
{
    if (!hasScanResult_) {
        return;
    }

    fileWorldLayout_ = fileWorldBuilder_.build(scanResult_, fileWorldSettings_);
    hasFileWorldLayout_ = true;
    selectedSceneId_ = InvalidSceneNodeId;
    hoveredSceneId_ = InvalidSceneNodeId;
    dangerDeleteWarningAccepted_ = false;
    clearSelectedImagePreview();
    applyFilters();
    appendScanLog("layout: " + std::string(fileWorldLayoutModeName(fileWorldSettings_.layoutMode)) + " // visible " + std::to_string(fileWorldLayout_.visibleCount));
    sceneMode_ = SceneMode::RealScan;
}

void App::selectFirstImageNode()
{
    if (!hasFileWorldLayout_) {
        return;
    }

    for (const SceneNode& sceneNode : fileWorldLayout_.nodes) {
        const FileNode* fileNode = fileNodeForSceneNode(&sceneNode);
        if (fileNode != nullptr && sceneNode.visible && fileNode->type == FileNodeType::File && fileNode->category == FileCategory::Image) {
            selectedSceneId_ = sceneNode.sceneId;
            const float radius = std::max({sceneNode.scale.x, sceneNode.scale.y, sceneNode.scale.z});
            startFocusAnimation(sceneNode.position, radius);
            appendScanLog("selected image preview: " + fileNode->name);
            return;
        }
    }

    appendScanLog("no image file found for preview");
}

void App::applyFilters()
{
    if (!hasFileWorldLayout_ || !hasScanResult_) {
        return;
    }

    std::size_t visibleCount = 0;
    for (SceneNode& sceneNode : fileWorldLayout_.nodes) {
        const FileNode* fileNode = fileNodeForSceneNode(&sceneNode);
        if (fileNode == nullptr) {
            sceneNode.visible = false;
            continue;
        }

        const bool matchesName = containsCaseInsensitive(fileNode->name, fileWorldFilters_.nameSubstring);
        const bool matchesExtension = containsCaseInsensitive(fileNode->extension, fileWorldFilters_.extensionSubstring);
        const bool matchesCategory = fileCategoryAllowed(fileNode->category, fileWorldFilters_);
        sceneNode.visible = matchesName && matchesExtension && matchesCategory;

        if (sceneNode.visible) {
            ++visibleCount;
        }
    }

    fileWorldLayout_.visibleCount = visibleCount;

    const SceneNode* selected = sceneNodeById(selectedSceneId_);
    if (selected != nullptr && !selected->visible) {
        selectedSceneId_ = InvalidSceneNodeId;
    }

    const SceneNode* hovered = sceneNodeById(hoveredSceneId_);
    if (hovered != nullptr && !hovered->visible) {
        hoveredSceneId_ = InvalidSceneNodeId;
    }
}

void App::clearFilters()
{
    fileWorldFilters_ = FileWorldFilters();
    applyFilters();
    appendScanLog("filters cleared");
}

void App::appendScanLog(std::string message)
{
    scanLog_.push_back(std::move(message));
    if (scanLog_.size() > 24) {
        scanLog_.erase(scanLog_.begin(), scanLog_.begin() + static_cast<std::ptrdiff_t>(scanLog_.size() - 24));
    }
}
