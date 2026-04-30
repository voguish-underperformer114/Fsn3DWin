#include "scene/DemoScene.hpp"

#include <algorithm>

DemoScene::DemoScene()
{
    build();
}

const std::vector<DemoBlock>& DemoScene::blocks() const
{
    return blocks_;
}

DemoPalette DemoScene::palette(DemoTheme theme)
{
    switch (theme) {
    case DemoTheme::AmberTerminal:
        return {
            .background = glm::vec3(0.004f, 0.006f, 0.003f),
            .gridTint = glm::vec4(1.0f, 0.74f, 0.16f, 1.0f),
            .scanWave = glm::vec4(1.0f, 0.12f, 0.04f, 0.40f),
            .root = glm::vec4(1.0f, 0.88f, 0.22f, 1.0f),
            .directoryA = glm::vec4(0.92f, 0.98f, 0.28f, 1.0f),
            .directoryB = glm::vec4(0.42f, 1.0f, 0.36f, 1.0f),
            .directoryC = glm::vec4(1.0f, 0.56f, 0.12f, 1.0f),
            .fileA = glm::vec4(1.0f, 0.72f, 0.18f, 1.0f),
            .fileB = glm::vec4(0.72f, 1.0f, 0.40f, 1.0f),
            .fileC = glm::vec4(1.0f, 0.94f, 0.55f, 1.0f),
            .core = glm::vec4(1.0f, 0.18f, 0.08f, 1.0f),
            .pulse = glm::vec4(1.0f, 0.38f, 0.06f, 1.0f),
            .gridIntensity = 1.56f,
        };
    case DemoTheme::JurassicSgi:
        return {
            .background = glm::vec3(0.003f, 0.010f, 0.020f),
            .gridTint = glm::vec4(0.62f, 1.0f, 0.86f, 1.0f),
            .scanWave = glm::vec4(0.15f, 1.0f, 0.74f, 0.34f),
            .root = glm::vec4(0.40f, 1.0f, 0.78f, 1.0f),
            .directoryA = glm::vec4(0.10f, 0.95f, 0.82f, 1.0f),
            .directoryB = glm::vec4(0.26f, 0.62f, 1.0f, 1.0f),
            .directoryC = glm::vec4(1.0f, 0.68f, 0.25f, 1.0f),
            .fileA = glm::vec4(0.22f, 0.82f, 1.0f, 1.0f),
            .fileB = glm::vec4(0.55f, 1.0f, 0.38f, 1.0f),
            .fileC = glm::vec4(1.0f, 0.86f, 0.22f, 1.0f),
            .core = glm::vec4(1.0f, 0.20f, 0.30f, 1.0f),
            .pulse = glm::vec4(1.0f, 0.18f, 0.92f, 1.0f),
            .gridIntensity = 1.32f,
        };
    case DemoTheme::NeonHacker:
        return {
            .background = glm::vec3(0.006f, 0.002f, 0.020f),
            .gridTint = glm::vec4(0.90f, 0.84f, 1.0f, 1.0f),
            .scanWave = glm::vec4(0.92f, 0.22f, 1.0f, 0.44f),
            .root = glm::vec4(0.04f, 1.0f, 1.0f, 1.0f),
            .directoryA = glm::vec4(0.02f, 0.78f, 1.0f, 1.0f),
            .directoryB = glm::vec4(0.90f, 0.28f, 1.0f, 1.0f),
            .directoryC = glm::vec4(1.0f, 0.18f, 0.62f, 1.0f),
            .fileA = glm::vec4(0.16f, 1.0f, 0.92f, 1.0f),
            .fileB = glm::vec4(1.0f, 0.36f, 0.95f, 1.0f),
            .fileC = glm::vec4(1.0f, 0.96f, 0.22f, 1.0f),
            .core = glm::vec4(1.0f, 0.10f, 0.20f, 1.0f),
            .pulse = glm::vec4(0.48f, 1.0f, 0.22f, 1.0f),
            .gridIntensity = 1.72f,
        };
    case DemoTheme::CleanDark:
    default:
        return {
            .background = glm::vec3(0.006f, 0.010f, 0.016f),
            .gridTint = glm::vec4(0.88f, 0.94f, 1.0f, 1.0f),
            .scanWave = glm::vec4(0.42f, 0.78f, 1.0f, 0.28f),
            .root = glm::vec4(0.78f, 0.92f, 1.0f, 1.0f),
            .directoryA = glm::vec4(0.45f, 0.78f, 1.0f, 1.0f),
            .directoryB = glm::vec4(0.58f, 0.92f, 0.75f, 1.0f),
            .directoryC = glm::vec4(0.94f, 0.70f, 0.42f, 1.0f),
            .fileA = glm::vec4(0.58f, 0.72f, 0.88f, 1.0f),
            .fileB = glm::vec4(0.55f, 0.84f, 0.66f, 1.0f),
            .fileC = glm::vec4(0.90f, 0.76f, 0.50f, 1.0f),
            .core = glm::vec4(1.0f, 0.45f, 0.48f, 1.0f),
            .pulse = glm::vec4(0.82f, 0.62f, 1.0f, 1.0f),
            .gridIntensity = 1.08f,
        };
    }
}

glm::vec4 DemoScene::colorFor(const DemoPalette& palette, DemoBlockCategory category)
{
    switch (category) {
    case DemoBlockCategory::Root:
        return palette.root;
    case DemoBlockCategory::DirectoryA:
        return palette.directoryA;
    case DemoBlockCategory::DirectoryB:
        return palette.directoryB;
    case DemoBlockCategory::DirectoryC:
        return palette.directoryC;
    case DemoBlockCategory::FileA:
        return palette.fileA;
    case DemoBlockCategory::FileB:
        return palette.fileB;
    case DemoBlockCategory::FileC:
        return palette.fileC;
    case DemoBlockCategory::Core:
        return palette.core;
    case DemoBlockCategory::Pulse:
    default:
        return palette.pulse;
    }
}

const char* DemoScene::themeName(DemoTheme theme)
{
    switch (theme) {
    case DemoTheme::AmberTerminal:
        return "Amber Terminal";
    case DemoTheme::JurassicSgi:
        return "Jurassic SGI";
    case DemoTheme::NeonHacker:
        return "Neon Hacker";
    case DemoTheme::CleanDark:
    default:
        return "Clean Dark";
    }
}

const char* DemoScene::sceneModeName(SceneMode mode)
{
    switch (mode) {
    case SceneMode::Demo:
        return "Demo Scene";
    case SceneMode::RealScan:
    default:
        return "Real Scan Scene";
    }
}

void DemoScene::build()
{
    blocks_.clear();
    blocks_.reserve(96);

    addBlock(0.0f, 0.0f, 2.8f, 10.5f, 2.8f, DemoBlockCategory::Root, 0.0f, 1.0f, true);

    addBlock(-8.0f, -7.0f, 1.8f, 4.5f, 1.8f, DemoBlockCategory::DirectoryA, 0.5f, 0.35f, true);
    addBlock(-12.0f, -5.0f, 1.5f, 3.4f, 1.5f, DemoBlockCategory::DirectoryA, 1.4f, 0.25f, false);
    addBlock(-7.5f, -12.0f, 1.6f, 5.2f, 1.6f, DemoBlockCategory::DirectoryA, 2.3f, 0.4f, true);

    addBlock(8.0f, -7.0f, 1.7f, 5.8f, 1.7f, DemoBlockCategory::DirectoryB, 1.1f, 0.35f, true);
    addBlock(12.0f, -4.0f, 1.3f, 3.2f, 1.3f, DemoBlockCategory::DirectoryB, 1.9f, 0.2f, false);
    addBlock(7.0f, -12.5f, 1.4f, 4.4f, 1.4f, DemoBlockCategory::DirectoryB, 2.7f, 0.32f, false);

    addBlock(-9.0f, 8.0f, 1.8f, 4.0f, 1.8f, DemoBlockCategory::DirectoryC, 0.8f, 0.32f, false);
    addBlock(-13.0f, 11.0f, 1.5f, 5.0f, 1.5f, DemoBlockCategory::DirectoryC, 1.8f, 0.35f, true);
    addBlock(-5.5f, 12.5f, 1.4f, 3.1f, 1.4f, DemoBlockCategory::DirectoryC, 2.5f, 0.18f, false);

    addBlock(8.5f, 8.5f, 1.4f, 12.8f, 1.4f, DemoBlockCategory::Core, 0.2f, 0.8f, true);
    addBlock(12.5f, 10.5f, 1.2f, 9.6f, 1.2f, DemoBlockCategory::Core, 1.2f, 0.62f, true);
    addBlock(10.0f, 14.5f, 1.1f, 8.0f, 1.1f, DemoBlockCategory::Core, 2.4f, 0.52f, true);
    addBlock(14.8f, 7.0f, 0.8f, 6.8f, 0.8f, DemoBlockCategory::Pulse, 3.3f, 0.72f, false);
    addBlock(5.5f, 15.8f, 0.9f, 5.9f, 0.9f, DemoBlockCategory::DirectoryB, 3.8f, 0.45f, false);

    addFileDistrict(-14.0f, -14.0f, 5, 4, DemoBlockCategory::FileA, 0.2f);
    addFileDistrict(6.0f, -15.0f, 6, 4, DemoBlockCategory::FileB, 1.4f);
    addFileDistrict(-15.0f, 3.5f, 5, 5, DemoBlockCategory::FileC, 2.2f);
    addFileDistrict(4.5f, 3.5f, 5, 5, DemoBlockCategory::Pulse, 3.0f);

    for (int i = 0; i < 14; ++i) {
        const float angle = static_cast<float>(i) * 0.4488f;
        const float x = std::clamp(angle * 3.2f - 9.5f, -11.5f, 11.5f);
        const float z = (i % 2 == 0 ? 1.0f : -1.0f) * (3.2f + static_cast<float>(i % 5));
        const float height = 0.7f + static_cast<float>((i * 7) % 5) * 0.28f;
        addBlock(x, z, 0.62f, height, 0.62f, DemoBlockCategory::Pulse, static_cast<float>(i) * 0.55f, 0.85f, false);
    }
}

void DemoScene::addBlock(
    float x,
    float z,
    float width,
    float height,
    float depth,
    DemoBlockCategory category,
    float pulsePhase,
    float pulseStrength,
    bool important)
{
    blocks_.push_back({
        .position = glm::vec3(x, height * 0.5f, z),
        .scale = glm::vec3(width, height, depth),
        .category = category,
        .pulsePhase = pulsePhase,
        .pulseStrength = pulseStrength,
        .important = important,
    });
}

void DemoScene::addFileDistrict(float originX, float originZ, int columns, int rows, DemoBlockCategory category, float phaseOffset)
{
    constexpr float spacing = 1.28f;

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float hash = static_cast<float>((row * 13 + column * 7) % 9);
            const float height = 0.45f + hash * 0.16f;
            const float width = 0.58f + static_cast<float>((row + column) % 3) * 0.08f;
            const float x = originX + static_cast<float>(column) * spacing;
            const float z = originZ + static_cast<float>(row) * spacing;
            const float pulse = (row + column) % 4 == 0 ? 0.55f : 0.15f;
            addBlock(x, z, width, height, width, category, phaseOffset + static_cast<float>(row + column) * 0.35f, pulse, false);
        }
    }
}
