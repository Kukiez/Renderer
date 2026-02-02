#include "ShaderStream.h"
#include "Types.h"
#include "ValueConstructor.h"


void ShaderStream::addAssignmentLine(std::string_view variable, const ShaderValueConstructor &value) {
    *this << variable.data() << " = " << value;
    return;
}

ShaderStream& ShaderStream::operator<<(const std::string_view &string_view) {
    result.append(string_view.data(), string_view.size());
    return *this;
}
