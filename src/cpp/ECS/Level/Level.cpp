#include "Level.h"
#include "ECS/Component/Types/PrimaryComponentType.h"
#include "ECS/Component/Types/SecondaryComponentType.h"
#include "ECS/Component/Types/BooleanComponentType.h"
#include "ECS/Component/Types/NameComponentType.h"

Level::Level(const std::string &name): ctx(name) {
    addComponentType<PrimaryComponentType>();
    addComponentType<SecondaryComponentType>();
    addComponentType<BooleanComponentType>();
    addComponentType<NameComponentType>();

    addStage<DefaultStage>();
}

void Level::initialize() {
    ctx.systemRegistry.createExecutionGraphs();
    synchronize();
    std::cout << "Level Started: " << ctx.lastFrame << std::endl;
}

void Level::run() {
    auto now = std::chrono::high_resolution_clock::now();
    runtime.runLevel(*this);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Level " << ctx.name << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - now).count() << " ns" << std::endl;
}

void Level::endFrame() {
    runtime.endFrame(*this);
}
