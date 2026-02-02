#include "OpenGLShaderCompiler.h"

#include <ranges>

#include "ShaderBuilder.h"
#include <regex>

static ShaderBuffer getBufferType(const std::string_view code) {
    if (code.contains("atomic_uint")) {
        return ShaderBuffer::AtomicUInt;
    }
    if (code.contains("uniform")) {
        return ShaderBuffer::Uniform;
    }
    if (code.contains("buffer")) {
        return ShaderBuffer::Storage;
    }
    return ShaderBuffer::Unknown;
}

ShaderDefinition OpenGLShaderCompiler::compileShaderFile(const ShaderCode& code) {
    ShaderBuilder builder(cache);

    const char* data = code.string().data();

    {
        std::string temp;
        static std::regex regex(R"((struct) (\w+))");

        auto begin = std::sregex_iterator(code.begin(), code.end(), regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            auto& match = *it;


            const char* nameBegin = match[2].first._Ptr;
            size_t nameLen = match[2].length();

            std::string_view name(nameBegin, nameLen);

            const ShaderClass* shaderClass = &cache.findShaderClass(name);

            if (!shaderClass->isNull()) {
                builder.addClass(*shaderClass);
                continue;
            }
            size_t structBegin = match[1].first._Ptr - code.string().data();
            size_t structEnd = code.string().find('}', structBegin);

            std::string_view structDef(data + structBegin, structEnd - structBegin + 2);
            shaderClass = &ShaderClass::fromString(cache, structDef);

            builder.addClass(*shaderClass);
        }
    }
    {
        std::string temp;
        static std::regex regex(R"(uniform\s+(\w+)\s+(\w+)\s*;)");

        auto begin = std::sregex_iterator(code.begin(), code.end(), regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            auto& match = *it;
            std::string_view type(match[1].first._Ptr, match[1].length());
            std::string_view name(match[2].first._Ptr, match[2].length());

            size_t uTypeHash = cexpr::type_hash(type.data(), type.length());

            auto typeEnum = getUniformType(uTypeHash);

            if (typeEnum == UniformParameterType::CLASS_TYPE) {
                // TODO handle class type
            } else {
                temp.assign(name);
                builder.addPushConstant(name, typeEnum);
            }
        }
    }

    {
        std::regex regex(R"(layout\s*\(\s*(?:(std140|std430)\s*,\s*)?binding\s*=\s*(\d+)\s*\)\s*(buffer|uniform|atomic)\s+(\w+))");

        auto begin = std::sregex_iterator(code.begin(), code.end(), regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            auto& match = *it;
            const std::string_view layout(match[1].first._Ptr, match[1].length());
            const std::string_view binding(match[2].first._Ptr, match[2].length());
            const std::string_view target(match[3].first._Ptr, match[3].length());
            const std::string_view name(match[4].first._Ptr, match[4].length());

            const auto buffer = getBufferType(target);

            int bindingInt;
            std::from_chars(binding.data(), binding.data() + binding.size(), bindingInt);

            BufferBindingIndex bindingIdx{bindingInt};

            ShaderBufferLayout layoutEnum;

            if (layout.empty()) {
                if (target.contains("uniform")) {
                    layoutEnum = ShaderBufferLayout::STD140;
                } else {
                    layoutEnum = ShaderBufferLayout::STD430;
                }
            } else {
                layoutEnum = layout.contains("std140") ? ShaderBufferLayout::STD140 : ShaderBufferLayout::STD430;
            }
            builder.addBuffer(name, buffer, bindingIdx, layoutEnum);
        }
    }
    return builder.create();
}

unsigned OpenGLShaderCompiler::createShader(const ShaderCode& code, ShaderProgramType type) {
    GLuint shader = glCreateShader(opengl_enum_cast(type));
    const char* src = code.string().data();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::string infoLog;
        infoLog.resize(1024);

        glGetShaderInfoLog(shader, infoLog.size(), nullptr, infoLog.data());
        std::cerr << "Failed to Compile: " << code.string() << std::endl;
        std::cerr <<  "ERROR::SHADER::COMPILATION_FAILED\n" + infoLog;

        for (auto rng : std::views::split(infoLog, '\n')) {
            std::string_view line(rng.data(), rng.size());

            size_t errorLine = line.find("0(");
            size_t errorLineEnd = line.find(')', errorLine);

            std::string_view error = line.substr(errorLine, errorLineEnd - errorLine);
            error.remove_prefix(2);

            int lineNum = std::stoi(std::string(error));

            size_t lineIdx = 0;

            for (int l = 0; l < lineNum; ++l) {
                lineIdx = code.string().find('\n', lineIdx) + 1;
            }
            std::cout << "Error on line: " << lineNum << " at: " << code.string().substr(std::max(0ull, lineIdx - 100), 100) << std::endl;
        }
        std::exit(-1);
    }
    return shader;
}

ShaderProgram OpenGLShaderCompiler::createShaderProgramDefinition(const ShaderPipelineDescriptor& shaders) {
    ShaderDefinition result;

    if (shaders.compute) {
        result += shaders.compute->definition();
    } else {
        if (shaders.fragment) {
            result += shaders.fragment->definition();
        }
        if (shaders.vertex) {
            result += shaders.vertex->definition();
        }
        if (shaders.geometry) {
            result += shaders.geometry->definition();
        }
        if (shaders.tessControl) {
            result += shaders.tessControl->definition();
        }
        if (shaders.tessEval) {
            result += shaders.tessEval->definition();
        }
    }
    auto& params = result.parameters().parameters();

    size_t offset = 0;
    for (size_t i = 0; i < params.size();) {
        params[i].location = -1;

        size_t align;
        const size_t size = getUniformSizeAndAlign(params[i].type, align);
        const size_t localOffset = mem::padding(offset, align);

        offset += localOffset;
        params[i].offset = offset;
        offset += size;
        ++i;
    }

    std::ranges::sort(params, [&](const auto& a, const auto& b) {
        return a.location < b.location;
    });
    params.shrink_to_fit();
    result.parameters().calculateTotalBytesRequired();
    return ShaderProgram(0, std::move(result), shaders);
}

ShaderProgram & OpenGLShaderCompiler::initializeShaderProgram(ShaderProgram &program,
    const ShaderPipelineDescriptor &shaders)
{
    ShaderProgram result;

    GLuint shaderProgram = glCreateProgram();

    if (shaders.compute) {
        glAttachShader(shaderProgram, shaders.compute->id());
    } else {
        if (shaders.fragment) {
            glAttachShader(shaderProgram, shaders.fragment->id());
        }
        if (shaders.vertex) {
            glAttachShader(shaderProgram, shaders.vertex->id());
        }
        if (shaders.geometry) {
            glAttachShader(shaderProgram, shaders.geometry->id());
        }
        if (shaders.tessControl) {
            glAttachShader(shaderProgram, shaders.tessControl->id());
        }
        if (shaders.tessEval) {
            glAttachShader(shaderProgram, shaders.tessEval->id());
        }
    }
    glLinkProgram(shaderProgram);
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::VALIDATION_FAILED\n" + std::string(infoLog);

        program = ShaderProgram(0, {}, shaders);

        std::cout << "Program Failed: " << shaders.compute << std::endl;
        std::cout << shaders.fragment << std::endl;
        assert(false);
        return program;
    }

    glValidateProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::INVALID_VALUE\n" + std::string(infoLog);
        program = ShaderProgram(0, {}, shaders);

        std::cout << "Program Failed: " << shaders.compute << std::endl;
        std::cout << shaders.fragment << std::endl;
        assert(false);
        return program;
    }

    auto& params = program.definition().parameters().parameters();

    for (size_t i = 0; i < params.size();) {
        if (const int loc = glGetUniformLocation(shaderProgram, params[i].name.string.data()); loc == -1) {
            std::cerr << "Parameter: " << params[i].name.string << " was removed by the GLSL Compiler." << std::endl;
            params.erase(params.begin() + i);
        } else {
            params[i].location = loc;
            ++i;
        }
    }

    program = ShaderProgram(shaderProgram, std::move(program.definition()), shaders);
    return program;
}