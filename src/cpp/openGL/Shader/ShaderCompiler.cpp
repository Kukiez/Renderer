#include "ShaderCompiler.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "ShaderCache.h"

std::string readFileImpl(const std::string_view filePath, std::unordered_set<std::string>& included, bool guardIncludes = false) {
    std::ifstream file((filePath.data()));
    if (!file) {
        std::cerr << "File not found: " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    std::string line;
    std::string result;
    std::filesystem::path basePath = std::filesystem::path(filePath).parent_path();

    while (std::getline(file, line)) {
        if (line.starts_with("#include")) {
            size_t firstQuote = line.find_first_of("\"<");
            size_t lastQuote  = line.find_last_of("\">");

            if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                std::string includePath = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                std::filesystem::path fullIncludePath = basePath / includePath;
                std::string include = fullIncludePath.string();
                std::cout << "Reading File: " << include << std::endl;

                auto filename = fullIncludePath.filename().string();

                if (included.contains(filename)) {
                    std::cerr << "Recursive include detected: " << filename << std::endl;
                    continue;
                }
                if (guardIncludes) included.insert("#ifndef " + filename + "\n");
                included.insert(std::move(filename));
                if (guardIncludes) included.insert("#endif\n");
                result += readFileImpl(include, included);
            }
        } else {
            if (auto idx = line.find_first_of('/'); idx != std::string::npos) {
                if (idx + 1 < line.size() && line[idx + 1] == '/') {
                    line = line.substr(0, idx);
                }
            }
            result += line + "\n";
        }
    }
    return result;
}

ShaderCode ShaderCode::open(std::string_view path) {
    const std::fstream file(path.data());

    if (!file.is_open()) {
        return ShaderCode("");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return ShaderCode(buffer.str());
}

std::string FileIncludeResolver::open(const Filepath &file) {
    std::unordered_set<std::string> included;
    return readFileImpl(file, included);
}

std::string FileIncludeResolver::openAndGuardIncludes(const Filepath &file) {
    std::unordered_set<std::string> included;
    return readFileImpl(file, included, true);
}

UniformParameterIndex ShaderProgram::getUniformParameter(size_t hash) const {
    return this->definition().parameters().getUniformParameter(hash);
}

UniformParameterIndex ShaderProgram::getUniformParameter(const ShaderString &str) const {
    return this->definition().parameters().getUniformParameter(str);
}

const ShaderUniformParameter * ShaderProgram::getUniform(UniformParameterIndex index) const {
    if (index == UniformParameterIndex::INVALID) return nullptr;
    return &this->definition().parameters()[index];
}

ShaderCode ShaderCompiler::parseShaderFile(std::string_view filePath) {
    std::unordered_set<std::string> included;
    return {readFileImpl(filePath, included)};
}

const Shader * ShaderPipelineBuilder::getOrCompileShader(const std::string_view path,
    const ShaderProgramType type) const {
    if (path.empty()) return nullptr;

    if (const auto* shader = cache.findShader(path)) {
        return shader;
    }
    const auto code = compiler.parseShaderFile(path);
    auto def  = compiler.compileShaderFile(code);
    const auto id   = compiler.createShader(code, type);

    Shader newShader(std::move(def), type, id);
    return &cache.addShader(path, std::move(newShader));
}

const ComputeShader * ShaderPipelineBuilder::getOrCompileShader(const std::string_view path,
    const glm::vec3 threads) const {
    if (path.empty()) return nullptr;

    if (const auto* shader = cache.findShader(path)) {
        return static_cast<const ComputeShader*>(shader);
    }
    const auto code = compiler.parseShaderFile(path);
    auto def  = compiler.compileShaderFile(code);
    const auto id   = compiler.createShader(code, ShaderProgramType::COMPUTE);

    return &cache.addComputeShader(path, ComputeShader(std::move(def), id, threads));

}
