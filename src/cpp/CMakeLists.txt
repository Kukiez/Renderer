cmake_minimum_required(VERSION 4.0)
project(Renderer)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE RENDERER_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(Renderer SHARED ${RENDERER_SOURCES}
        Renderer/Renderer.h
        Renderer/Graphics/Passes/FullScreenPass.h
        Renderer/Graphics/Passes/FullScreenPass.cpp
)

target_include_directories(Renderer
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Ext
)

target_include_directories(Renderer PRIVATE C:/Main/vcpkg/vcpkg/installed/x64-windows/include/openexr)
target_include_directories(Renderer PRIVATE C:/Main/vcpkg/vcpkg/installed/x64-windows/include/Imath)

set(CMAKE_PREFIX_PATH "C:/Main/vcpkg/vcpkg/installed/x64-windows" ${CMAKE_PREFIX_PATH})

find_package(TBB CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Imath CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(Renderer PRIVATE glm::glm)

target_link_libraries(Renderer PRIVATE Imath::Imath)
target_link_libraries(Renderer PRIVATE assimp::assimp)
target_link_libraries(Renderer PRIVATE SDL2::SDL2 SDL2::SDL2main)

file(GLOB LIBS "${PROJECT_SOURCE_DIR}/../lib/*.lib")
target_link_libraries(Renderer PRIVATE ${LIBS})

target_link_libraries(Renderer PRIVATE OpenGL::GL)
target_link_libraries(Renderer
        PRIVATE TBB::tbb
)

target_compile_definitions(Renderer PRIVATE RENDERER_BUILD)