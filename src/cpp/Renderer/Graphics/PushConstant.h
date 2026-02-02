#pragma once
#include <bitset>
#include <string_view>
#include <openGL/Shader/ShaderReflection.h>

struct ShaderUniformParameter;
struct GraphicsAllocator;

enum class UniformParameterIndex : unsigned;

class GraphicsContext;
struct ShaderString;
class ShaderProgram;

enum class UniformParameterType;

class PushConstantSet {
    void push(const UniformParameterIndex& index, const void* push, size_t pushBytes);
    UniformParameterType push(const ShaderString& str, const void* push, size_t pushBytes);

    const ShaderProgram* shader{};
    char* block{};
    std::bitset<64> usedSlots{};
public:
    PushConstantSet(GraphicsAllocator* allocator, const ShaderProgram* shader) : PushConstantSet(allocate(allocator, shader)) {}

    PushConstantSet(const ShaderProgram* shader, char* block) : shader(shader), block(block) {}

    static PushConstantSet allocate(GraphicsAllocator* allocator, const ShaderProgram* shader);

    template <typename T>
    void push(std::string_view name, const T& value);

    bool bind(const GraphicsContext& graphics) const;

    static bool bind(const GraphicsContext& graphics, const ShaderProgram* shader, const ShaderUniformParameter& type, const void* data);
    static bool bind(const GraphicsContext& graphics, const ShaderProgram* shader, const UniformParameterIndex& uniform, const void* data);
    static bool bind(const GraphicsContext& graphics, const ShaderProgram* shader, std::string_view uniform, const void* data);
    static bool bind(const GraphicsContext& graphics, const ShaderProgram* shader, const ShaderString& uniform, const void* data);
};
