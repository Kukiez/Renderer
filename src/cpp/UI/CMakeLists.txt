add_library(UI SHARED)

target_sources(UI PRIVATE
        UITypeContext.cpp
        UIContext.cpp
        Window/UIWindowContext.cpp
        WIndow/UIAnimationContext.cpp
        Window/LayoutContext.cpp
        Text/UIFontLoader.cpp
        Rendering/UI.cpp
        Rendering/UIRenderingContext.cpp
)

target_include_directories(UI
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(UI PRIVATE UI_API_BUILD)

target_link_libraries(UI PUBLIC Image)
target_link_libraries(UI PUBLIC Math)
target_link_libraries(UI PUBLIC RendererCore)
target_link_libraries(UI PUBLIC ECS)

find_package(Freetype CONFIG REQUIRED)
target_link_libraries(UI PRIVATE Freetype::Freetype)