add_library(Image SHARED)

target_sources(Image PRIVATE
    Image.cpp
    ImageLoader.cpp
)

target_include_directories(Image
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(OpenEXR CONFIG REQUIRED)
find_package(Imath CONFIG REQUIRED)
find_package(Stb REQUIRED)

target_include_directories(Image PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(Image PRIVATE Imath::Imath)
target_link_libraries(Image PRIVATE OpenEXR::OpenEXR)

target_compile_definitions(Image PRIVATE IMAGE_API_BUILD)