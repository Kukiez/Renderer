#include "ComponentRegistry.h"

struct NullComponentType {};

struct StringEq {
    bool operator () (const char* a, const char* b) const {
        return strcmp(a, b) == 0;
    }
};

struct StringHash {
    size_t operator () (const char* a) const {
        return std::hash<std::string_view>{}(a);
    }
};

struct ComponentTypeRegistry::Impl {
    std::unordered_map<const char*, ComponentIndex, StringHash, StringEq> stableIndices;
    std::unordered_map<const char*, ComponentKind, StringHash, StringEq> typeToKind;

    std::vector<unsigned> nextComponentIndices;

    struct Header {
        const char* name{};
    };

    tbb::concurrent_vector<Header> kindToHeader;

    std::mutex lock;

    ComponentKind nextKind() const {
        return static_cast<ComponentKind>(static_cast<uint16_t>(nextComponentIndices.size()));
    }
};

ComponentTypeRegistry::Impl& getImpl() {
    static ComponentTypeRegistry::Impl impl;
    return impl;
}

ComponentIndex ComponentTypeRegistry::getOrCreateComponentIndex(const ComponentKind kind, const char* name) {
    auto& impl = getImpl();

    std::lock_guard lock(impl.lock);

    const auto it = impl.stableIndices.find(name);

    if (it != impl.stableIndices.end()) {
        return it->second;
    }

    const int kindInt = static_cast<int>(kind);

    auto& nextIndex = impl.nextComponentIndices[kindInt];

    ComponentIndex index(nextIndex++);

    impl.stableIndices.emplace(name, index);

    std::cout << "Registering Component: " << name << " to Index: " << index.id() << " in Category: " << impl.kindToHeader[kind.id()].name << std::endl;
    return index;
}

ComponentKind ComponentTypeRegistry::getComponentKind(const char* name) {
    auto& impl = getImpl();

    std::lock_guard lock(impl.lock);

    auto& typeToKind = impl.typeToKind;
    const auto it = typeToKind.find(name);

    if (it != impl.typeToKind.end()) {
        return it->second;
    }

    auto kind = impl.nextKind();

    if (kind.id() == 0) {
        const auto nullType = cexpr::name_of<NullComponentType>;
        impl.typeToKind.emplace(nullType, kind);
        impl.nextComponentIndices.emplace_back(1);

        std::cout << "Registering ComponentKind/Type: " << nullType << " to Kind: " << kind.id() << std::endl;

        impl.kindToHeader.emplace_back(nullType);
        kind = impl.nextKind();
    }

    impl.typeToKind.emplace(name, kind);
    impl.nextComponentIndices.emplace_back(1);

    std::cout << "Registering ComponentKind/Type: " << name << " to Kind: " << kind.id() << std::endl;

    impl.kindToHeader.emplace_back(name);
    return kind;
}

void ComponentTypeRegistry::initializeZeroComponentIndex(ComponentKind kind, const char* name) {
    auto& impl = getImpl();

    std::lock_guard lock(impl.lock);

    const auto it = impl.stableIndices.find(name);

    if (it != impl.stableIndices.end()) {
        if (it->first != name) {
            assert(false);
        }
        return;
    }

    impl.stableIndices.emplace(name, ComponentIndex(0));
    std::cout << "Registering Zero ComponentIndex for Type: " << it->first << " with Type: " << name << std::endl;
}

std::ostream & operator<<(std::ostream &os, ComponentKind kind) {
    os << mem::Typename(getImpl().kindToHeader[kind.id()].name);
    return os;
}

std::ostream & operator<<(std::ostream &os, const TypeUUID type) {
    os << "{Kind: " << type.kind() << ", ID: " << type.id() << "}";
    return os;
}
