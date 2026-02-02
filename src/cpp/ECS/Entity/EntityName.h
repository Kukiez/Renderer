#pragma once
#include <constexpr/General.h>

class NameQuery;
class ComponentFactory;
class NameComponentType;

class EntityName {
    constexpr static char UNNAMED[] = "unnamed";
    constexpr static auto UNNAMED_LENGTH = cexpr::strlen(UNNAMED);
    const char* myName = UNNAMED;
    size_t myLength = UNNAMED_LENGTH;
    size_t myHash = 0;

    std::string_view makeStringView() const {
        return {
            myName, myLength
        };
    }
public:
    using ComponentType = NameComponentType;
    using FactoryType = ComponentFactory;
    using QueryType = NameQuery;

    EntityName() = default;

    EntityName(const char* name, const size_t length, const size_t hash) : myName(name), myLength(length), myHash(hash) {}

    explicit EntityName(const std::string_view name) : myName(name.data()), myLength(name.length()), myHash(std::hash<std::string_view>{}(name)) {}

    operator std::string_view() const {
        return makeStringView();
    }

    std::string_view name() const {
        return makeStringView();
    }

    size_t hash() const {
        return myHash;
    }

    friend std::ostream& operator << (std::ostream& os, const EntityName& e) {
        os << e.makeStringView();
        return os;
    }

    size_t size() const {
        return myLength;
    }

    size_t length() const {
        return myLength;
    }

    const char* data() const {
        return myName;
    }

    bool isNamed() const {
        return myName != UNNAMED;
    }
};
