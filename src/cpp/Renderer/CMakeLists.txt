
add_library(RendererCore SHARED
        Bloom/BloomPass.cpp
        Bloom/BloomRenderer.cpp
        Display/DisplayPass.cpp
        Light/LightSystem.cpp
        Light/BRDFLutBakePass.cpp
        RenderingStages/OpaquePass.cpp
        Scene/Primitives/PrimitiveStorage.cpp
        Scene/Worlds/MipWorld.cpp
        Skybox/SkyboxBakePass.cpp
        Skybox/SkyboxPass.cpp
        Skybox/SkyboxSystem.cpp
        SSAO/SSAO.cpp
        Renderer.cpp
        SDLWindow.cpp
)

file(GLOB_RECURSE RENDERER_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Resource/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../openGL/*.cpp
)

target_sources(RendererCore PRIVATE
        ${RENDERER_SOURCES}
        Graphics/PushConstant.cpp
        Graphics/PassBindings.cpp
        Graphics/RenderPass.cpp
        Graphics/DrawContainers/DrawContainers.cpp
        Graphics/Features/Features.cpp
        Graphics/Passes/EnvCubemapPass.cpp
        Graphics/Passes/FullScreenPass.cpp
        Graphics/Passes/Passes.cpp
        Graphics/Textures/Cubemap.cpp
        Graphics/Textures/Texture.cpp
        Graphics/Textures/Texture2D.cpp
        Graphics/Textures/Texture2DMS.cpp
        Graphics/UtilityPasses/UtilityPasses.cpp
        Graphics/GraphicsContext.cpp
        Graphics/GraphicsPass.cpp
)

target_include_directories(RendererCore
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(RendererCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_compile_definitions(RendererCore PRIVATE RENDERER_API_BUILD)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

target_link_libraries(RendererCore PUBLIC SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(RendererCore PUBLIC OpenGL::GL GLEW::GLEW)

target_link_libraries(RendererCore PUBLIC Image)
target_link_libraries(RendererCore PUBLIC Math)
target_link_libraries(RendererCore PUBLIC ECS)

target_compile_definitions(RendererCore PRIVATE RENDERER_API_BUILD)