#pragma once

class WorldCullCallback {
public:
    using CallbackFn = void(*)(const void* inst, PrimitiveCollectionID collection);
private:
    void* instance;
    CallbackFn callback;
public:
    WorldCullCallback(void* instance, CallbackFn callback) : instance(instance), callback(callback) {}

    operator bool() const { return callback; }

    void operator () (PrimitiveCollectionID collection) const {
        callback(instance, collection);
    }
};