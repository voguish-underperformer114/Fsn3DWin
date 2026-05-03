#include "ui/Hud.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>

namespace {
constexpr int ThemeCount = 4;
constexpr int RenderModeCount = 3;
constexpr int LayoutModeCount = 3;
constexpr int SizeEmphasisCount = 2;
constexpr int LabelModeCount = 4;

ImVec4 legendColor(FileCategory category)
{
    switch (category) {
    case FileCategory::Directory:
        return ImVec4(1.0f, 0.80f, 0.18f, 1.0f);
    case FileCategory::Source:
        return ImVec4(0.45f, 1.0f, 0.36f, 1.0f);
    case FileCategory::Document:
        return ImVec4(1.0f, 0.93f, 0.42f, 1.0f);
    case FileCategory::Image:
        return ImVec4(1.0f, 0.52f, 0.12f, 1.0f);
    case FileCategory::Audio:
        return ImVec4(0.92f, 0.84f, 0.22f, 1.0f);
    case FileCategory::Video:
        return ImVec4(1.0f, 0.22f, 0.08f, 1.0f);
    case FileCategory::Archive:
        return ImVec4(0.72f, 1.0f, 0.24f, 1.0f);
    case FileCategory::Executable:
        return ImVec4(1.0f, 0.10f, 0.04f, 1.0f);
    case FileCategory::System:
        return ImVec4(1.0f, 0.76f, 0.28f, 1.0f);
    case FileCategory::Data:
        return ImVec4(0.58f, 1.0f, 0.44f, 1.0f);
    case FileCategory::Error:
        return ImVec4(1.0f, 0.08f, 0.08f, 1.0f);
    case FileCategory::Other:
    default:
        return ImVec4(0.86f, 0.68f, 0.34f, 1.0f);
    }
}

const char* categoryGuideName(FileCategory category)
{
    switch (category) {
    case FileCategory::Directory:
        return "Folder";
    case FileCategory::Source:
        return "Code file";
    case FileCategory::Document:
        return "Document";
    case FileCategory::Image:
        return "Image file";
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
    case FileCategory::Other:
    default:
        return "Other file";
    }
}

std::string formatBytes(std::uintmax_t bytes)
{
    constexpr double KiB = 1024.0;
    constexpr double MiB = KiB * 1024.0;
    constexpr double GiB = MiB * 1024.0;

    char buffer[64]{};
    if (bytes >= static_cast<std::uintmax_t>(GiB)) {
        std::snprintf(buffer, sizeof(buffer), "%.2f GiB", static_cast<double>(bytes) / GiB);
    } else if (bytes >= static_cast<std::uintmax_t>(MiB)) {
        std::snprintf(buffer, sizeof(buffer), "%.2f MiB", static_cast<double>(bytes) / MiB);
    } else if (bytes >= static_cast<std::uintmax_t>(KiB)) {
        std::snprintf(buffer, sizeof(buffer), "%.2f KiB", static_cast<double>(bytes) / KiB);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%llu B", static_cast<unsigned long long>(bytes));
    }

    return buffer;
}

void drawMessageList(const char* title, const std::vector<std::string>& messages)
{
    if (messages.empty()) {
        return;
    }

    ImGui::TextUnformatted(title);

    const std::size_t first = messages.size() > 4 ? messages.size() - 4 : 0;
    for (std::size_t index = first; index < messages.size(); ++index) {
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextWrapped("%s", messages[index].c_str());
    }
}

void drawLegendItem(FileCategory category)
{
    ImGui::ColorButton(fileCategoryName(category), legendColor(category), ImGuiColorEditFlags_NoTooltip, ImVec2(13.0f, 13.0f));
    ImGui::SameLine();
    ImGui::Text("%s - %s", fileCategoryName(category), categoryGuideName(category));
}

std::string pathToString(const std::filesystem::path& path)
{
    try {
        return path.string();
    } catch (...) {
        return "<path>";
    }
}

std::string compactText(const std::string& text, std::size_t maxLength)
{
    if (text.size() <= maxLength) {
        return text;
    }

    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }

    return text.substr(0, maxLength - 3) + "...";
}

bool overlapsExistingRect(const std::vector<ImVec4>& rects, const ImVec4& candidate, float padding)
{
    return std::any_of(rects.begin(), rects.end(), [&](const ImVec4& rect) {
        return candidate.x < rect.z + padding &&
            candidate.z > rect.x - padding &&
            candidate.y < rect.w + padding &&
            candidate.w > rect.y - padding;
    });
}

void setReadableHudWindowSize(float preferredWidth)
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float availableWidth = std::max(280.0f, display.x - 36.0f);
    const float minWidth = std::min(preferredWidth, availableWidth);
    const float maxHeight = std::max(260.0f, display.y - 36.0f);
    ImGui::SetNextWindowSize(ImVec2(preferredWidth, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(minWidth, 0.0f),
        ImVec2(availableWidth, maxHeight));
}

void drawSelectedMetadata(const SceneNode* selectedSceneNode, const FileNode* selectedFileNode)
{
    if (selectedSceneNode == nullptr || selectedFileNode == nullptr) {
        ImGui::TextDisabled("No selected node");
        return;
    }

    ImGui::TextUnformatted("Selected metadata");
    ImGui::Separator();
    ImGui::TextWrapped("Name: %s", selectedFileNode->name.c_str());
    ImGui::Text("What it is: %s", fileNodeKindName(*selectedFileNode));
    ImGui::TextWrapped("Full path: %s", pathToString(selectedFileNode->fullPath).c_str());
    ImGui::Text("Technical type: %s", fileNodeTypeName(selectedFileNode->type));
    ImGui::Text("Extension: %s", selectedFileNode->extension.empty() ? "(none)" : selectedFileNode->extension.c_str());
    ImGui::Text("Category: %s", fileCategoryName(selectedFileNode->category));
    ImGui::Text(
        "Size: %s",
        selectedFileNode->sizeKnown ? formatBytes(selectedFileNode->sizeBytes).c_str() : "(unknown)");
    ImGui::Text("Depth: %u", selectedFileNode->depth);

    if (selectedFileNode->type == FileNodeType::Directory) {
        ImGui::Text("Child count: %llu", static_cast<unsigned long long>(selectedFileNode->childIds.size()));
    }

    ImGui::Text("Scene id: %u", selectedSceneNode->sceneId);
}

void drawImagePreviewPanel(const HudState& state, bool showStatus)
{
    ImGui::Separator();
    ImGui::TextUnformatted("Image preview");

    if (state.imagePreviewTextureId == 0 || state.imagePreviewWidth <= 0 || state.imagePreviewHeight <= 0) {
        if (!state.imagePreviewStatus.empty()) {
            ImGui::TextWrapped("%s", state.imagePreviewStatus.c_str());
        } else {
            ImGui::TextDisabled("No image file inspected");
        }
        return;
    }

    const float availableWidth = std::max(180.0f, ImGui::GetContentRegionAvail().x);
    const float aspect = static_cast<float>(state.imagePreviewWidth) / static_cast<float>(std::max(state.imagePreviewHeight, 1));
    float previewWidth = std::min(availableWidth, 430.0f);
    float previewHeight = previewWidth / std::max(aspect, 0.05f);
    if (previewHeight > 220.0f) {
        previewHeight = 220.0f;
        previewWidth = previewHeight * aspect;
    }

    previewWidth = std::clamp(previewWidth, 96.0f, availableWidth);
    previewHeight = std::clamp(previewHeight, 72.0f, 220.0f);

    const float offsetX = (availableWidth - previewWidth) * 0.5f;
    if (offsetX > 0.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
    }

    const ImTextureID texture = (ImTextureID)(std::intptr_t)state.imagePreviewTextureId;
    ImGui::Image(texture, ImVec2(previewWidth, previewHeight));

    if (showStatus && !state.imagePreviewStatus.empty()) {
        ImGui::TextWrapped("%s", state.imagePreviewStatus.c_str());
    }
}

void drawQuickInspector(
    const HudState& state,
    const SceneNode* selectedSceneNode,
    const FileNode* selectedFileNode)
{
    const bool hasSelected = selectedSceneNode != nullptr && selectedFileNode != nullptr;
    const bool hasHover = state.hoverTooltipVisible && !state.hoverName.empty();
    const bool hasPreview = state.imagePreviewTextureId != 0;
    if (!hasSelected && !hasHover && !hasPreview) {
        return;
    }

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x < 840.0f || display.y < 420.0f) {
        return;
    }

    const float width = std::min(360.0f, std::max(300.0f, display.x * 0.28f));
    const float height = hasPreview ? 400.0f : 230.0f;
    const float clampedHeight = std::min(height, std::max(220.0f, display.y - 168.0f));
    ImGui::SetNextWindowPos(ImVec2(display.x - width - 18.0f, 112.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, clampedHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.93f);

    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
    if (hasSelected) {
        ImGui::TextWrapped("%s", selectedFileNode->name.c_str());
        ImGui::Text("What it is: %s", fileNodeKindName(*selectedFileNode));
        ImGui::Text("Category: %s", fileCategoryName(selectedFileNode->category));
        ImGui::Text(
            "Size: %s",
            selectedFileNode->sizeKnown ? formatBytes(selectedFileNode->sizeBytes).c_str() : "(unknown)");
        if (selectedFileNode->type == FileNodeType::Directory) {
            ImGui::Text("Contains: %llu items", static_cast<unsigned long long>(selectedFileNode->childIds.size()));
        }
    } else {
        ImGui::TextWrapped("%s", state.hoverName.c_str());
        if (!state.hoverCategory.empty()) {
            ImGui::TextWrapped("%s", state.hoverCategory.c_str());
        }
        if (!state.hoverSize.empty()) {
            ImGui::TextWrapped("%s", state.hoverSize.c_str());
        }
    }

    drawImagePreviewPanel(state, false);
    ImGui::End();
}

void drawActiveNamePlate(const HudState& state, const FileNode* selectedFileNode)
{
    std::string title;
    std::string subtitle;
    bool selected = false;

    if (selectedFileNode != nullptr) {
        selected = true;
        title = selectedFileNode->name;
        subtitle = std::string(fileNodeKindName(*selectedFileNode)) + " // " + pathToString(selectedFileNode->fullPath);
    } else if (state.hoverTooltipVisible && !state.hoverName.empty()) {
        title = state.hoverName;
        subtitle = state.hoverCategory;
    }

    if (title.empty()) {
        return;
    }

    title = compactText(title, 72);
    subtitle = compactText(subtitle, 128);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImFont* font = ImGui::GetFont();
    const float baseFontSize = ImGui::GetFontSize();
    const float titleScale = selected ? 1.28f : 1.20f;
    const float subtitleScale = 1.06f;
    const ImVec2 measuredTitleSize = ImGui::CalcTextSize(title.c_str());
    const ImVec2 measuredSubtitleSize = ImGui::CalcTextSize(subtitle.c_str());
    const ImVec2 titleSize(measuredTitleSize.x * titleScale, measuredTitleSize.y * titleScale);
    const ImVec2 subtitleSize(measuredSubtitleSize.x * subtitleScale, measuredSubtitleSize.y * subtitleScale);
    const float leftReserve = display.x > 880.0f ? 520.0f : 0.0f;
    const float availableMinX = leftReserve + 18.0f;
    const float availableWidth = std::max(300.0f, display.x - availableMinX - 24.0f);
    const float width = std::min(std::max(titleSize.x, subtitleSize.x) + 48.0f, availableWidth);
    const float height = subtitle.empty() ? 52.0f : 88.0f;
    const ImVec2 min(availableMinX + availableWidth * 0.5f - width * 0.5f, 20.0f);
    const ImVec2 max(min.x + width, min.y + height);
    const ImU32 border = selected ? IM_COL32(255, 226, 66, 240) : IM_COL32(255, 118, 36, 220);

    drawList->AddRectFilled(min, max, IM_COL32(5, 6, 2, 226), 5.0f);
    drawList->AddRect(min, max, border, 5.0f, 0, 1.4f);
    drawList->AddText(
        font,
        baseFontSize * titleScale,
        ImVec2(min.x + 24.0f, min.y + 12.0f),
        selected ? IM_COL32(255, 244, 150, 255) : IM_COL32(255, 198, 88, 255),
        title.c_str());
    if (!subtitle.empty()) {
        drawList->AddText(
            font,
            baseFontSize * subtitleScale,
            ImVec2(min.x + 24.0f, min.y + 53.0f),
            IM_COL32(192, 255, 138, 235),
            subtitle.c_str());
    }
}

void drawScreenLabels(const HudState& state)
{
    if (state.overlayLabels.empty()) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 padding(12.0f, 8.0f);
    std::vector<ImVec4> usedRects;
    std::vector<ImVec4> blockedRects;
    usedRects.reserve(state.overlayLabels.size());
    blockedRects.reserve(3);

    const float hudWidth = std::min(std::max(0.0f, display.x - 16.0f), state.cleanHud ? 520.0f : 540.0f);
    const float hudHeight = state.cleanHud ? 190.0f : display.y - 18.0f;
    blockedRects.push_back(ImVec4(12.0f, 12.0f, hudWidth, hudHeight));

    if (display.x > 720.0f) {
        const float plateHalfWidth = std::min(430.0f, display.x * 0.34f);
        blockedRects.push_back(ImVec4(display.x * 0.5f - plateHalfWidth, 8.0f, display.x * 0.5f + plateHalfWidth, 104.0f));
    }

    const bool sparseLabelSet = state.overlayLabels.size() <= 36;
    for (const OverlayLabel& label : state.overlayLabels) {
        if (label.text.empty()) {
            continue;
        }

        const std::string text = compactText(label.text, label.accent ? 76 : 54);
        ImFont* font = ImGui::GetFont();
        const float baseFontSize = ImGui::GetFontSize();
        const float fontScale = label.accent ? 1.22f : 1.10f;
        const ImVec2 measuredSize = ImGui::CalcTextSize(text.c_str());
        const ImVec2 textSize(measuredSize.x * fontScale, measuredSize.y * fontScale);
        const ImVec2 anchor(label.position.x, label.position.y);
        ImVec2 min(anchor.x - textSize.x * 0.5f - padding.x, anchor.y - textSize.y - padding.y * 2.0f);
        ImVec2 max(anchor.x + textSize.x * 0.5f + padding.x, anchor.y);
        const float labelWidth = max.x - min.x;
        const float labelHeight = max.y - min.y;
        const float shiftX = std::clamp(min.x, 8.0f, std::max(8.0f, display.x - labelWidth - 8.0f)) - min.x;
        const float shiftY = std::clamp(min.y, 8.0f, std::max(8.0f, display.y - labelHeight - 82.0f)) - min.y;
        min.x += shiftX;
        max.x += shiftX;
        min.y += shiftY;
        max.y += shiftY;

        const ImVec4 rect(min.x, min.y, max.x, max.y);
        if (!label.accent && !sparseLabelSet && overlapsExistingRect(blockedRects, rect, 4.0f)) {
            continue;
        }
        if (!label.accent && !sparseLabelSet && overlapsExistingRect(usedRects, rect, 6.0f)) {
            continue;
        }
        usedRects.push_back(rect);

        const ImU32 borderColor = label.accent ? IM_COL32(255, 226, 70, 238) : IM_COL32(255, 150, 48, 220);

        drawList->AddRectFilled(min, max, IM_COL32(5, 7, 2, label.accent ? 238 : 216), 4.0f);
        drawList->AddRect(min, max, borderColor, 4.0f, 0, label.accent ? 1.6f : 1.1f);
        drawList->AddLine(ImVec2(anchor.x, max.y), ImVec2(anchor.x, max.y + 10.0f), borderColor, 1.0f);
        drawList->AddText(
            font,
            baseFontSize * fontScale,
            ImVec2(min.x + padding.x + 1.0f, min.y + padding.y + 1.0f),
            IM_COL32(0, 0, 0, 230),
            text.c_str());
        drawList->AddText(
            font,
            baseFontSize * fontScale,
            ImVec2(min.x + padding.x, min.y + padding.y),
            label.accent ? IM_COL32(255, 245, 150, 255) : IM_COL32(240, 255, 166, 255),
            text.c_str());
    }
}

void drawDangerActions(
    HudAction& action,
    bool& dangerActionsEnabled,
    bool& dangerDeleteWarningAccepted,
    const FileNode* selectedFileNode,
    std::array<char, 32>& deleteConfirmBuffer)
{
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.22f, 1.0f), "Danger zone");
    ImGui::TextWrapped("Default mode is read-only. These actions can open files or move a file to the Recycle Bin.");

    if (ImGui::Checkbox("Warning 1: enable dangerous actions", &dangerActionsEnabled) && !dangerActionsEnabled) {
        dangerDeleteWarningAccepted = false;
        std::fill(deleteConfirmBuffer.begin(), deleteConfirmBuffer.end(), '\0');
    }

    const bool hasSelection = selectedFileNode != nullptr;
    const bool canOpen = dangerActionsEnabled && hasSelection;
    if (!canOpen) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Open selected with Windows")) {
        action = HudAction::OpenSelected;
    }
    if (!canOpen) {
        ImGui::EndDisabled();
    }

    const bool canDeleteType = hasSelection && selectedFileNode->type == FileNodeType::File;
    if (!dangerActionsEnabled || !canDeleteType) {
        ImGui::BeginDisabled();
    }

    ImGui::Checkbox("Warning 2: I understand delete moves the selected file", &dangerDeleteWarningAccepted);
    ImGui::InputText("Type DELETE", deleteConfirmBuffer.data(), deleteConfirmBuffer.size());

    const bool deletePhraseMatches = std::strcmp(deleteConfirmBuffer.data(), "DELETE") == 0;
    const bool canDelete = dangerActionsEnabled && canDeleteType && dangerDeleteWarningAccepted && deletePhraseMatches;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Move selected file to Recycle Bin")) {
        action = HudAction::DeleteSelected;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (!dangerActionsEnabled || !canDeleteType) {
        ImGui::EndDisabled();
    }

    if (hasSelection && selectedFileNode->type == FileNodeType::Directory) {
        ImGui::TextDisabled("Directory delete is intentionally unavailable.");
    }
}

void drawCategoryToggles(FileWorldFilters& filters)
{
    ImGui::Checkbox("Directories", &filters.showDirectories);
    ImGui::SameLine();
    ImGui::Checkbox("Code", &filters.showCode);

    ImGui::Checkbox("Images", &filters.showImages);
    ImGui::SameLine();
    ImGui::Checkbox("Video", &filters.showVideo);

    ImGui::Checkbox("Audio", &filters.showAudio);
    ImGui::SameLine();
    ImGui::Checkbox("Documents", &filters.showDocuments);

    ImGui::Checkbox("Archives", &filters.showArchives);
    ImGui::SameLine();
    ImGui::Checkbox("Executables", &filters.showExecutables);

    ImGui::Checkbox("Other", &filters.showOther);
}

void drawHoverTooltip(const HudState& state)
{
    if (!state.hoverTooltipVisible || state.hoverName.empty()) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 padding(12.0f, 8.0f);
    const ImVec2 origin(state.hoverTooltipPosition.x + 18.0f, state.hoverTooltipPosition.y + 18.0f);
    const char* lines[] = {
        state.hoverName.c_str(),
        state.hoverCategory.c_str(),
        state.hoverSize.c_str(),
    };

    ImVec2 size(0.0f, 0.0f);
    for (const char* line : lines) {
        const ImVec2 lineSize = ImGui::CalcTextSize(line);
        size.x = std::max(size.x, lineSize.x);
        size.y += lineSize.y + 3.0f;
    }
    size.y -= 3.0f;

    const ImVec2 min = origin;
    const ImVec2 max(origin.x + size.x + padding.x * 2.0f, origin.y + size.y + padding.y * 2.0f);
    drawList->AddRectFilled(min, max, IM_COL32(5, 7, 2, 232), 5.0f);
    drawList->AddRect(min, max, IM_COL32(255, 148, 36, 232), 5.0f);

    float y = min.y + padding.y;
    drawList->AddText(ImVec2(min.x + padding.x, y), IM_COL32(255, 236, 128, 255), lines[0]);
    y += ImGui::GetTextLineHeight() + 3.0f;
    drawList->AddText(ImVec2(min.x + padding.x, y), IM_COL32(184, 255, 128, 245), lines[1]);
    y += ImGui::GetTextLineHeight() + 3.0f;
    drawList->AddText(ImVec2(min.x + padding.x, y), IM_COL32(255, 126, 66, 245), lines[2]);
}

void drawCornerBracket(ImDrawList* drawList, const ImVec2& a, float size, bool right, bool bottom, ImU32 color)
{
    const float sx = right ? -size : size;
    const float sy = bottom ? -size : size;
    drawList->AddLine(a, ImVec2(a.x + sx, a.y), color, 1.6f);
    drawList->AddLine(a, ImVec2(a.x, a.y + sy), color, 1.6f);
}

void drawOverlayEffects(const HudState& state, const FileScanOptions& scanOptions, const FileScanResult* scanResult)
{
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(state.totalSeconds) * 2.3f);
    const ImU32 bracketColor = IM_COL32(255, 154, 36, static_cast<int>(140 + pulse * 75));

    for (float y = 0.0f; y < display.y; y += 4.0f) {
        drawList->AddLine(ImVec2(0.0f, y), ImVec2(display.x, y), IM_COL32(255, 198, 88, 9), 1.0f);
    }

    const float margin = 18.0f;
    const float bracket = 42.0f + pulse * 8.0f;
    drawCornerBracket(drawList, ImVec2(margin, margin), bracket, false, false, bracketColor);
    drawCornerBracket(drawList, ImVec2(display.x - margin, margin), bracket, true, false, bracketColor);
    drawCornerBracket(drawList, ImVec2(margin, display.y - margin), bracket, false, true, bracketColor);
    drawCornerBracket(drawList, ImVec2(display.x - margin, display.y - margin), bracket, true, true, bracketColor);

    const std::string rootPath = "ROOT: " + pathToString(scanOptions.rootPath);
    drawList->AddText(ImVec2(24.0f, display.y - 62.0f), IM_COL32(216, 255, 138, 225), rootPath.c_str());
    drawList->AddText(ImVec2(24.0f, display.y - 36.0f), IM_COL32(255, 222, 96, 235), "SAFE MODE DEFAULT // OPEN+DELETE REQUIRE DANGER WARNINGS");

    if (state.scanState == FileScanState::Scanning || state.scanState == FileScanState::CancelRequested) {
        const float sweep = std::fmod(static_cast<float>(state.totalSeconds) * 220.0f, display.x + 260.0f) - 130.0f;
        drawList->AddRectFilled(
            ImVec2(sweep - 80.0f, 0.0f),
            ImVec2(sweep + 80.0f, display.y),
            IM_COL32(255, 80, 24, 24));
        drawList->AddText(
            ImVec2(display.x * 0.5f - 118.0f, 28.0f),
            IM_COL32(255, 220, 100, 238),
            state.scanState == FileScanState::CancelRequested ? "CANCEL REQUESTED // DRAINING WORKER" : "INDEXING SECTORS // READ-ONLY SCAN");
    }

    const std::vector<std::string>* log = state.scanLog;
    const bool logEmpty = log == nullptr || log->empty();
    const bool useFallbackLog = scanResult != nullptr && logEmpty;

    if (scanResult == nullptr && logEmpty &&
        state.scanState != FileScanState::Scanning &&
        state.scanState != FileScanState::CancelRequested) {
        return;
    }

    const ImVec2 logMin(display.x - 430.0f, display.y - 188.0f);
    const ImVec2 logMax(display.x - 24.0f, display.y - 28.0f);
    drawList->AddRectFilled(logMin, logMax, IM_COL32(4, 6, 2, 176), 4.0f);
    drawList->AddRect(logMin, logMax, IM_COL32(255, 134, 32, 148), 4.0f);
    drawList->AddText(ImVec2(logMin.x + 12.0f, logMin.y + 10.0f), IM_COL32(255, 206, 88, 228), "SCAN LOG");

    char fallbackCount[80]{};
    if (useFallbackLog) {
        std::snprintf(fallbackCount, sizeof(fallbackCount), "visible nodes: %llu", static_cast<unsigned long long>(scanResult->counts.nodes));
    }

    const std::size_t logSize = useFallbackLog ? 2U : (log != nullptr ? log->size() : 0U);
    const std::size_t first = logSize > 5 ? logSize - 5 : 0;
    float y = logMin.y + 38.0f;
    for (std::size_t index = first; index < logSize; ++index) {
        const char* line = "";
        if (useFallbackLog) {
            line = index == 0 ? "awaiting filter or selection input" : fallbackCount;
        } else if (log != nullptr) {
            line = (*log)[index].c_str();
        }

        drawList->AddText(ImVec2(logMin.x + 12.0f, y), IM_COL32(210, 255, 152, 214), line);
        y += ImGui::GetTextLineHeight() + 4.0f;
    }
}
}

const char* labelModeName(LabelMode mode)
{
    switch (mode) {
    case LabelMode::Off:
        return "Off";
    case LabelMode::SelectedOnly:
        return "Selected only";
    case LabelMode::Important:
        return "Important";
    case LabelMode::LimitedAll:
    default:
        return "Limited all";
    }
}

const char* presentationCameraModeName(PresentationCameraMode mode)
{
    switch (mode) {
    case PresentationCameraMode::OrbitRoot:
        return "Orbit root";
    case PresentationCameraMode::OrbitSelected:
        return "Orbit selected";
    case PresentationCameraMode::FlyThrough:
    default:
        return "Slow fly-through";
    }
}

HudAction Hud::draw(
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
    const FileNode* selectedFileNode)
{
    HudAction action = HudAction::None;

    if (cleanHud) {
        ImGui::SetNextWindowPos(ImVec2(18.0f, 18.0f), ImGuiCond_FirstUseEver);
        setReadableHudWindowSize(470.0f);
        ImGui::Begin("Fsn3DWin", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted("Fsn3DWin");
        ImGui::Text("Version %s", state.appVersion.c_str());
        ImGui::TextUnformatted("Cinematic 3D filesystem visualizer");
        ImGui::Separator();
        ImGui::Text("Presentation: %s", presentationActive ? presentationCameraModeName(presentationMode) : "off");
        ImGui::Text("Read-only mode: ON");
        if (!state.screenshotStatus.empty()) {
            ImGui::TextWrapped("%s", state.screenshotStatus.c_str());
        }
        ImGui::Separator();
        drawSelectedMetadata(selectedSceneNode, selectedFileNode);
        drawImagePreviewPanel(state, true);
        ImGui::End();

        drawQuickInspector(state, selectedSceneNode, selectedFileNode);
        drawOverlayEffects(state, scanOptions, scanResult);
        drawScreenLabels(state);
        drawActiveNamePlate(state, selectedFileNode);
        drawHoverTooltip(state);
        return action;
    }

    ImGui::SetNextWindowPos(ImVec2(18.0f, 18.0f), ImGuiCond_FirstUseEver);
    setReadableHudWindowSize(480.0f);

    ImGui::Begin("Fsn3DWin", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextUnformatted("Fsn3DWin");
    ImGui::Text("Version %s", state.appVersion.c_str());
    ImGui::TextUnformatted("Cinematic 3D filesystem visualizer");
    ImGui::Separator();
    ImGui::TextUnformatted(state.stageName);
    ImGui::Text("FPS: %.1f", state.fps);
    ImGui::Text("Frame time: %.2f ms", state.frameTimeMs);
    ImGui::Text("Instances: %llu", static_cast<unsigned long long>(state.instanceCount));
    ImGui::Text("Scan state: %s", fileScanStateName(state.scanState));
    if (state.scanState == FileScanState::Scanning || state.scanState == FileScanState::CancelRequested) {
        ImGui::Text(
            "Progress: %llu nodes  %llu dirs  %llu warnings  %.2f s",
            static_cast<unsigned long long>(state.scanNodes),
            static_cast<unsigned long long>(state.scanDirectories),
            static_cast<unsigned long long>(state.scanWarnings),
            state.scanElapsedSeconds);
    }
    ImGui::Text("OpenGL version: %s", state.openGlVersion.c_str());
    ImGui::Text("Read-only mode: %s", state.readOnly ? "ON" : "OFF");
    ImGui::Separator();

    int selectedTheme = static_cast<int>(theme);
    const char* themePreview = DemoScene::themeName(theme);
    if (ImGui::BeginCombo("Theme", themePreview)) {
        for (int index = 0; index < ThemeCount; ++index) {
            const auto candidate = static_cast<DemoTheme>(index);
            const bool selected = selectedTheme == index;
            if (ImGui::Selectable(DemoScene::themeName(candidate), selected)) {
                selectedTheme = index;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    theme = static_cast<DemoTheme>(selectedTheme);

    int selectedRenderMode = static_cast<int>(renderMode);
    if (ImGui::BeginCombo("Render mode", renderModeName(renderMode))) {
        for (int index = 0; index < RenderModeCount; ++index) {
            const auto candidate = static_cast<RenderMode>(index);
            const bool selected = selectedRenderMode == index;
            if (ImGui::Selectable(renderModeName(candidate), selected)) {
                selectedRenderMode = index;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    renderMode = static_cast<RenderMode>(selectedRenderMode);

    ImGui::Separator();
    ImGui::TextUnformatted("Presentation");
    ImGui::Checkbox("Presentation mode", &presentationActive);
    ImGui::SameLine();
    ImGui::Checkbox("Clean HUD", &cleanHud);

    int selectedPresentationMode = static_cast<int>(presentationMode);
    if (ImGui::BeginCombo("Camera mode", presentationCameraModeName(presentationMode))) {
        for (int index = 0; index < 3; ++index) {
            const auto candidate = static_cast<PresentationCameraMode>(index);
            const bool selected = selectedPresentationMode == index;
            if (ImGui::Selectable(presentationCameraModeName(candidate), selected)) {
                selectedPresentationMode = index;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    presentationMode = static_cast<PresentationCameraMode>(selectedPresentationMode);

    ImGui::Checkbox("Pause camera", &presentationPaused);
    ImGui::SliderFloat("Presentation speed", &presentationSpeed, 0.15f, 4.0f, "%.2f");
    if (!state.screenshotStatus.empty()) {
        ImGui::TextWrapped("%s", state.screenshotStatus.c_str());
    }

    int selectedLabelMode = static_cast<int>(labelMode);
    if (ImGui::BeginCombo("Label mode", labelModeName(labelMode))) {
        for (int index = 0; index < LabelModeCount; ++index) {
            const auto candidate = static_cast<LabelMode>(index);
            const bool selected = selectedLabelMode == index;
            if (ImGui::Selectable(labelModeName(candidate), selected)) {
                selectedLabelMode = index;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    labelMode = static_cast<LabelMode>(selectedLabelMode);

    int selectedMode = static_cast<int>(sceneMode);
    if (ImGui::RadioButton("Demo Scene", selectedMode == static_cast<int>(SceneMode::Demo))) {
        selectedMode = static_cast<int>(SceneMode::Demo);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Real Scan Scene", selectedMode == static_cast<int>(SceneMode::RealScan))) {
        selectedMode = static_cast<int>(SceneMode::RealScan);
    }
    sceneMode = static_cast<SceneMode>(selectedMode);

    ImGui::Separator();
    syncRootPathBuffer(scanOptions);
    syncFilterBuffers(fileWorldFilters);

    if (ImGui::InputText("Root path", rootPathBuffer_.data(), rootPathBuffer_.size())) {
        bufferedRootPath_ = rootPathBuffer_.data();
        scanOptions.rootPath = bufferedRootPath_;
    }

    int maxDepth = static_cast<int>(scanOptions.maxDepth);
    if (ImGui::InputInt("Max depth", &maxDepth)) {
        maxDepth = std::clamp(maxDepth, 0, 32);
        scanOptions.maxDepth = static_cast<std::uint32_t>(maxDepth);
    }

    int maxNodes = static_cast<int>(std::min<std::size_t>(scanOptions.maxNodes, 100000));
    if (ImGui::InputInt("Max nodes", &maxNodes, 100, 1000)) {
        maxNodes = std::clamp(maxNodes, 1, 100000);
        scanOptions.maxNodes = static_cast<std::size_t>(maxNodes);
    }

    ImGui::Checkbox("Include hidden", &scanOptions.includeHidden);

    if (state.scanState == FileScanState::Scanning || state.scanState == FileScanState::CancelRequested) {
        ImGui::BeginDisabled(state.scanState == FileScanState::CancelRequested);
        if (ImGui::Button("Cancel scan")) {
            action = HudAction::CancelScan;
        }
        ImGui::EndDisabled();
    } else if (ImGui::Button("Scan")) {
        action = HudAction::Scan;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Search filters");
    if (ImGui::InputText("Name", nameFilterBuffer_.data(), nameFilterBuffer_.size())) {
        bufferedNameFilter_ = nameFilterBuffer_.data();
        fileWorldFilters.nameSubstring = bufferedNameFilter_;
    }

    if (ImGui::InputText("Extension", extensionFilterBuffer_.data(), extensionFilterBuffer_.size())) {
        bufferedExtensionFilter_ = extensionFilterBuffer_.data();
        fileWorldFilters.extensionSubstring = bufferedExtensionFilter_;
    }

    drawCategoryToggles(fileWorldFilters);

    if (ImGui::Button("Clear filters")) {
        action = HudAction::ClearFilters;
    }

    if (scanResult != nullptr) {
        const FileScanCounts& counts = scanResult->counts;
        ImGui::Text("Scan time: %.3f s", scanResult->elapsedSeconds);
        ImGui::Text(
            "Nodes: %llu  Dirs: %llu  Files: %llu",
            static_cast<unsigned long long>(counts.nodes),
            static_cast<unsigned long long>(counts.directories),
            static_cast<unsigned long long>(counts.files));
        ImGui::Text(
            "Symlinks: %llu  Other: %llu  Errors: %llu",
            static_cast<unsigned long long>(counts.symlinks),
            static_cast<unsigned long long>(counts.other),
            static_cast<unsigned long long>(counts.errors));
        ImGui::Text("Known size: %s", formatBytes(counts.knownBytes).c_str());

        if (scanResult->truncated) {
            ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.18f, 1.0f), "Result truncated at max nodes");
        }

        drawMessageList("Warnings", scanResult->warnings);
        drawMessageList("Errors", scanResult->errors);

        ImGui::Separator();
        ImGui::TextUnformatted("File world layout");
        int selectedLayoutMode = static_cast<int>(fileWorldSettings.layoutMode);
        if (ImGui::BeginCombo("Layout mode", fileWorldLayoutModeName(fileWorldSettings.layoutMode))) {
            for (int index = 0; index < LayoutModeCount; ++index) {
                const auto candidate = static_cast<FileWorldLayoutMode>(index);
                const bool selected = selectedLayoutMode == index;
                if (ImGui::Selectable(fileWorldLayoutModeName(candidate), selected)) {
                    selectedLayoutMode = index;
                    action = HudAction::RebuildLayout;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        fileWorldSettings.layoutMode = static_cast<FileWorldLayoutMode>(selectedLayoutMode);

        int selectedSizeMode = static_cast<int>(fileWorldSettings.sizeEmphasis);
        if (ImGui::BeginCombo("Size emphasis", sizeEmphasisModeName(fileWorldSettings.sizeEmphasis))) {
            for (int index = 0; index < SizeEmphasisCount; ++index) {
                const auto candidate = static_cast<SizeEmphasisMode>(index);
                const bool selected = selectedSizeMode == index;
                if (ImGui::Selectable(sizeEmphasisModeName(candidate), selected)) {
                    selectedSizeMode = index;
                    action = HudAction::RebuildLayout;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        fileWorldSettings.sizeEmphasis = static_cast<SizeEmphasisMode>(selectedSizeMode);

        ImGui::SliderFloat("Spacing", &fileWorldSettings.spacing, 0.75f, 4.0f, "%.2f");
        ImGui::SliderFloat("Height scale", &fileWorldSettings.heightScale, 0.15f, 1.6f, "%.2f");
        ImGui::SliderFloat("Directory scale", &fileWorldSettings.directoryTowerScale, 0.5f, 2.2f, "%.2f");
        ImGui::SliderFloat("Max height", &fileWorldSettings.maxHeight, 4.0f, 36.0f, "%.1f");
        ImGui::SliderFloat("City spread", &fileWorldSettings.citySpread, 0.45f, 2.8f, "%.2f");

        if (ImGui::Button("Rebuild layout")) {
            action = HudAction::RebuildLayout;
        }

        if (fileWorldLayout != nullptr) {
            ImGui::Text(
                "Scene nodes: %llu  Visible: %llu",
                static_cast<unsigned long long>(fileWorldLayout->nodes.size()),
                static_cast<unsigned long long>(fileWorldLayout->visibleCount));
        }

        if (ImGui::TreeNodeEx("Category legend", ImGuiTreeNodeFlags_DefaultOpen)) {
            drawLegendItem(FileCategory::Directory);
            drawLegendItem(FileCategory::Source);
            drawLegendItem(FileCategory::Document);
            drawLegendItem(FileCategory::Image);
            drawLegendItem(FileCategory::Audio);
            drawLegendItem(FileCategory::Video);
            drawLegendItem(FileCategory::Archive);
            drawLegendItem(FileCategory::Executable);
            drawLegendItem(FileCategory::System);
            drawLegendItem(FileCategory::Data);
            drawLegendItem(FileCategory::Other);
            drawLegendItem(FileCategory::Error);
            ImGui::TreePop();
        }
    }

    ImGui::Separator();
    drawSelectedMetadata(selectedSceneNode, selectedFileNode);
    drawImagePreviewPanel(state, true);
    drawDangerActions(action, dangerActionsEnabled, dangerDeleteWarningAccepted, selectedFileNode, deleteConfirmBuffer_);

    ImGui::Separator();
    ImGui::Text(
        "Camera position: %.2f, %.2f, %.2f",
        state.cameraPosition.x,
        state.cameraPosition.y,
        state.cameraPosition.z);
    ImGui::Text("Yaw / pitch: %.1f / %.1f", state.cameraYaw, state.cameraPitch);
    ImGui::Text("Speed: %.1f", state.cameraSpeed);

    if (ImGui::Button("Reset camera")) {
        action = HudAction::ResetCamera;
    }

    ImGui::SameLine();

    if (ImGui::Button("Quit")) {
        action = HudAction::Quit;
    }

    ImGui::End();
    drawQuickInspector(state, selectedSceneNode, selectedFileNode);
    drawOverlayEffects(state, scanOptions, scanResult);
    drawScreenLabels(state);
    drawActiveNamePlate(state, selectedFileNode);
    drawHoverTooltip(state);

    return action;
}

void Hud::syncRootPathBuffer(const FileScanOptions& scanOptions)
{
    std::string rootPath;
    try {
        rootPath = scanOptions.rootPath.string();
    } catch (...) {
        rootPath = ".";
    }

    if (rootPathBufferReady_ && bufferedRootPath_ == rootPath) {
        return;
    }

    bufferedRootPath_ = rootPath;
    std::fill(rootPathBuffer_.begin(), rootPathBuffer_.end(), '\0');
    const std::size_t copyLength = std::min(rootPath.size(), rootPathBuffer_.size() - 1U);
    std::copy_n(rootPath.data(), copyLength, rootPathBuffer_.data());
    rootPathBufferReady_ = true;
}

void Hud::syncFilterBuffers(const FileWorldFilters& fileWorldFilters)
{
    if (filterBuffersReady_ &&
        bufferedNameFilter_ == fileWorldFilters.nameSubstring &&
        bufferedExtensionFilter_ == fileWorldFilters.extensionSubstring) {
        return;
    }

    bufferedNameFilter_ = fileWorldFilters.nameSubstring;
    bufferedExtensionFilter_ = fileWorldFilters.extensionSubstring;

    std::fill(nameFilterBuffer_.begin(), nameFilterBuffer_.end(), '\0');
    std::fill(extensionFilterBuffer_.begin(), extensionFilterBuffer_.end(), '\0');

    const std::size_t nameLength = std::min(bufferedNameFilter_.size(), nameFilterBuffer_.size() - 1U);
    const std::size_t extensionLength = std::min(bufferedExtensionFilter_.size(), extensionFilterBuffer_.size() - 1U);

    std::copy_n(bufferedNameFilter_.data(), nameLength, nameFilterBuffer_.data());
    std::copy_n(bufferedExtensionFilter_.data(), extensionLength, extensionFilterBuffer_.data());
    filterBuffersReady_ = true;
}
