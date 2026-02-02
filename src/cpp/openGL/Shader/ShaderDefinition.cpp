#include "ShaderDefinition.h"

std::pair<BufferBindingIndex, int> ShaderDefinition::getBufferBinding(const std::string_view buffer) const {
    for (size_t i = 0; i < myBuffers.size(); ++i) {
        if (myBuffers[i].name == buffer) {
            return {myBuffers[i].binding, static_cast<int>(i)};
        }
    }
    return {BufferBindingIndex::INVALID, -1};
}

ShaderDefinition & ShaderDefinition::operator+=(const ShaderDefinition &other) {
    myBuffers.reserve(myBuffers.size() + other.myBuffers.size());
    uniformLayout.myParameters.reserve(uniformLayout.myParameters.size() + other.uniformLayout.myParameters.size());
    myClasses.reserve(myClasses.size() + other.myClasses.size());

    for (const auto& buf : other.myBuffers) {
        if (std::ranges::find(myBuffers, buf) == myBuffers.end()) {
            myBuffers.emplace_back(buf);
        }
    }

    const auto& otherParams = other.parameters();
    const auto& selfParams  = parameters();

    for (const auto& uniform : otherParams) {
        if (std::ranges::find(selfParams, uniform) == selfParams.end()) {
            uniformLayout.myParameters.emplace_back(uniform);
        }
    }

    for (const auto& cls : other.myClasses) {
        if (std::ranges::find(myClasses, cls) == myClasses.end()) {
            myClasses.emplace_back(cls);
        }
    }

    return *this;
}

std::ostream & operator<<(std::ostream &os, const ShaderDefinition &def) {
    os << "ShaderDefinition: \n  ";
    std::cout << " Classes: \n";
    for (const auto& cls : def.myClasses) {
        os << *cls << "\n";
    }
    std::cout << " Buffers: \n ";
    for (const auto& buffer : def.myBuffers) {
        os << buffer << "\n ";
    }
    std::cout << " Parameters: \n  ";
    for (const auto& param : def.parameters()) {
        os << param << "\n";
    }
    return os;
}
