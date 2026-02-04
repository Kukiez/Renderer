add_library(Mesh SHARED)

target_sources(Mesh PRIVATE
        ModelCooking.cpp
        ModelResource.cpp
        Animation/ModelAnimation.cpp
)

target_include_directories(Mesh
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(Mesh PRIVATE MESH_API_BUILD)

find_package(TBB CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

target_link_libraries(Mesh PRIVATE TBB::tbb)
target_link_libraries(Mesh PRIVATE Image)
target_link_libraries(Mesh PRIVATE Math)
target_link_libraries(Mesh PRIVATE assimp::assimp)
target_link_libraries(Mesh PRIVATE RendererCore)
target_link_libraries(Mesh PRIVATE ECS)