#include "RenderingPassKey.h"

ecs::TypeContext<RenderingPassType::Ctx> RenderingPassType::ctx;

void RenderingPassType::Ctx::onFlush() {
    if (nameToPassPending.empty()) return;

    passToTable.resize(nameToPassPending.size() + passToTable.size());

    for (auto& [table, typeID] : nameToPassPending) {
        nameToPass.emplace(table.name, typeID);
        passToTable[typeID.id()] = table;
    }
    nameToPassPending.clear();
}

RenderingPassType::Ctx::TypeID RenderingPassType::Ctx::findPassByName(const std::string_view name) {
    const auto it = nameToPass.find(name);

    if (it != nameToPass.end()) {
        return it->second;
    }
    for (auto& [pName, typeID] : nameToPassPending) {
        if (pName.name == name) return typeID;
    }
    return {};
}
