#pragma once
#include <string>
#include <gl/glew.h>
#include <memory/byte_arena.h>
#include <Util/glm_double.h>

#include "ShaderDefinition.h"
#include "ShaderReflection.h"

enum class ShaderProgramType {
    INVALID = 0,
    VERTEX = 1,
    FRAGMENT = 2,
    GEOMETRY = 3,
    TESS_CONTROL = 4,
    TESS_EVALUATION = 5,
    COMPUTE = 6,
    MAX_PROGRAMS = 6
};

constexpr GLenum opengl_enum_cast(ShaderProgramType type) {
    switch (type) {
        case ShaderProgramType::VERTEX:
            return GL_VERTEX_SHADER;
        case ShaderProgramType::FRAGMENT:
            return GL_FRAGMENT_SHADER;
        case ShaderProgramType::GEOMETRY:
            return GL_GEOMETRY_SHADER;
        case ShaderProgramType::TESS_CONTROL:
            return GL_TESS_CONTROL_SHADER;
        case ShaderProgramType::TESS_EVALUATION:
            return GL_TESS_EVALUATION_SHADER;
        case ShaderProgramType::COMPUTE:
            return GL_COMPUTE_SHADER;
    }
    return GL_INVALID_ENUM;
};

enum class ShaderProgramSpec {
    INVALID = 0,
    VERTEX = 1 << 0,
    FRAGMENT = 1 << 1,
    TESS_CONTROL = 1 << 2,
    TESS_EVALUATION = 1 << 3,
    COMPUTE = 1 << 4,
    MAX_PROGRAMS = 4
};

class Shader {
protected:
    ShaderDefinition def;
    ShaderProgramType programType = ShaderProgramType::INVALID;
    unsigned uuid = 0;
public:
    explicit Shader(ShaderDefinition&& definition, const ShaderProgramType programs, const unsigned id) : def(std::move(definition)), programType(programs),uuid(id) {}

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept
    : def(std::move(other.def)), programType(other.programType), uuid(other.uuid)
    {
        other.programType = ShaderProgramType::INVALID;
        other.uuid = 0;
    }

    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            def = std::move(other.def);
            programType = other.programType;
            uuid = other.uuid;
            other.programType = ShaderProgramType::INVALID;
            other.uuid = 0;
        }
        return *this;
    }

    bool operator == (const Shader& other) const {
        return uuid == other.uuid;
    }

    auto& definition() const {
        return def;
    }

    friend std::ostream& operator<<(std::ostream& os, const Shader& shader) {
        os << shader.def;
        return os;
    }

    ShaderProgramType program() const {
        return programType;
    }

    unsigned id() const {
        return uuid;
    }
};

static inline auto nullshader = Shader(ShaderDefinition{}, ShaderProgramType::INVALID, 0);

class ComputeShader : public Shader {
    glm::vec3 threads;
public:
    ComputeShader(ShaderDefinition&& def, const unsigned id, const glm::vec3 threads) : Shader(std::move(def), ShaderProgramType::COMPUTE, id), threads(threads) {}

    const glm::vec3& getThreads() const {
        return threads;
    }

    void dispatch(const size_t amount) const {
        const int num_groups_x = static_cast<int>((amount + threads.x - 1) / threads.x);

        glDispatchCompute(num_groups_x, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    friend std::ostream& operator<<(std::ostream& os, const ComputeShader& shader) {
        os << shader.def;
        std::cout << " Threads: " << shader.threads << "\n";
        return os;
    }
};

struct ShaderPipelineDescriptor {
    const Shader* vertex = nullptr;
    const Shader* fragment = nullptr;
    const Shader* geometry = nullptr;
    const Shader* tessControl = nullptr;
    const Shader* tessEval = nullptr;
    const ComputeShader* compute = nullptr;
};