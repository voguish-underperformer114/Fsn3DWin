cmake_minimum_required(VERSION 3.24)

project(Fsn3DWin
    VERSION 0.1.0
    DESCRIPTION "A cinematic 3D filesystem visualizer for Windows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(Fsn3DWin
    src/main.cpp
    src/app/App.cpp
    src/app/App.hpp
    src/filesystem/FileNode.cpp
    src/filesystem/FileNode.hpp
    src/filesystem/FileScanner.cpp
    src/filesystem/FileScanner.hpp
    src/scene/Camera.cpp
    src/scene/Camera.hpp
    src/scene/DemoScene.cpp
    src/scene/DemoScene.hpp
    src/scene/FileWorldBuilder.cpp
    src/scene/FileWorldBuilder.hpp
    src/scene/SceneNode.cpp
    src/scene/SceneNode.hpp
    src/renderer/InstancedCubeRenderer.cpp
    src/renderer/InstancedCubeRenderer.hpp
    src/renderer/Mesh.cpp
    src/renderer/Mesh.hpp
    src/renderer/Renderer.cpp
    src/renderer/Renderer.hpp
    src/renderer/Shader.cpp
    src/renderer/Shader.hpp
    src/ui/Hud.cpp
    src/ui/Hud.hpp
    src/util/Timer.cpp
    src/util/Timer.hpp
)

target_include_directories(Fsn3DWin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(Fsn3DWin PRIVATE
    FSN3DWIN_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(Fsn3DWin PRIVATE
    glfw
    glad::glad
    glm::glm
    imgui::imgui
    OpenGL::GL
)

if (MSVC)
    target_compile_options(Fsn3DWin PRIVATE /W4 /permissive-)
else()
    target_compile_options(Fsn3DWin PRIVATE -Wall -Wextra -Wpedantic)
endif()
