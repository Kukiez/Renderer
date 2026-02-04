
file(GLOB_RECURSE ECS_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(ECS SHARED
        ${ECS_SOURCES}
        ECSAPI.h
)

target_include_directories(ECS PUBLIC
        ${CMAKE_SOURCE_DIR}
)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(ECS PUBLIC TBB::tbb)

target_compile_definitions(ECS PRIVATE ECS_API_BUILD)