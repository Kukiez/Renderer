
add_library(Math SHARED
        GlmPrint.h)

target_sources(Math PRIVATE
        Shapes/geom.cpp
        Shapes/Ray.cpp
)

target_include_directories(Math
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(Math PRIVATE MATH_API_BUILD)

find_package(glm CONFIG REQUIRED)
target_link_libraries(Math PUBLIC glm::glm)