#pragma once

struct UpdateSystemFnTable {
    void* system = nullptr;
    UpdateFn update = nullptr;

    UpdateSystemFnTable() = default;

    UpdateSystemFnTable(const UpdateSystemDescriptor& usd, void* instance) {
        update = usd.update;
        system = instance;
    }
};