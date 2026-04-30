#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vector>

enum class DemoTheme {
    JurassicSgi = 0,
    NeonHacker,
    CleanDark,
};

enum class SceneMode {
    Demo = 0,
    RealScan,
};

enum class DemoBlockCategory {
    Root,
    DirectoryA,
    DirectoryB,
    DirectoryC,
    FileA,
    FileB,
    FileC,
    Core,
    Pulse,
};

struct DemoBlock {
    glm::vec3 position{};
    glm::vec3 scale{1.0f};
    DemoBlockCategory category = DemoBlockCategory::FileA;
    float pulsePhase = 0.0f;
    float pulseStrength = 0.0f;
    bool important = false;
};

struct DemoPalette {
    glm::vec3 background{};
    glm::vec4 gridTint{};
    glm::vec4 scanWave{};
    glm::vec4 root{};
    glm::vec4 directoryA{};
    glm::vec4 directoryB{};
    glm::vec4 directoryC{};
    glm::vec4 fileA{};
    glm::vec4 fileB{};
    glm::vec4 fileC{};
    glm::vec4 core{};
    glm::vec4 pulse{};
    float gridIntensity = 1.0f;
};

class DemoScene {
public:
    DemoScene();

    const std::vector<DemoBlock>& blocks() const;

    static DemoPalette palette(DemoTheme theme);
    static glm::vec4 colorFor(const DemoPalette& palette, DemoBlockCategory category);
    static const char* themeName(DemoTheme theme);
    static const char* sceneModeName(SceneMode mode);

private:
    void build();
    void addBlock(
        float x,
        float z,
        float width,
        float height,
        float depth,
        DemoBlockCategory category,
        float pulsePhase = 0.0f,
        float pulseStrength = 0.0f,
        bool important = false);
    void addFileDistrict(float originX, float originZ, int columns, int rows, DemoBlockCategory category, float phaseOffset);

    std::vector<DemoBlock> blocks_;
};
