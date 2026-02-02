#include "MaterialCooking.h"

#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <openGL/Shader/ShaderClass.h>
#include <openGL/Shader/ShaderCompiler.h>

static constexpr auto OutputPath = Filepath("Saved/Materials/");

std::string CreateFragMain(std::string_view materialName) {
    std::string result;

    result += "layout(std430, binding = 25) buffer Materials { \n";
    result += materialName;
    result += " materials[]; \n";
    result += "};\n\n in flat uint vMaterialID;\n\n";
    result += "void main() {\n    pass_main(material_main(materials[vMaterialID])); \n}";
    return result;
}

std::string slurp(std::fstream& in) {
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

LargeFilepath MaterialCooking_t::createFragmentShaderPermutation(const Filepath &vertex, const Filepath &material, const Filepath &pass) {
    LargeFilepath materialPath = OutputPath;
    materialPath += vertex.stem();
    materialPath += "/";
    materialPath += material.stem();
    materialPath += "/";
    materialPath += material.stem();
    materialPath += ".glsl";

    Filepath passFilePath = OutputPath + "Passes/" + pass.stem() + ".glsl";

    std::fstream materialFile(materialPath.data(), std::ios::in);
    std::fstream passFile(passFilePath.data(), std::ios::in);

    std::string matContent = slurp(materialFile);
    std::string passContent = slurp(passFile);

    std::string merged = passContent + matContent;

    std::filesystem::create_directories(OutputPath.data());

    LargeFilepath mergedPaths = OutputPath + vertex.stem();
    mergedPaths += "/";
    mergedPaths += material.stem();
    mergedPaths += "/Passes/";
    mergedPaths += pass.stem();
    std::fstream outFragFile((mergedPaths + ".glsl").data(), std::ios::trunc | std::ios::out | std::ios::in);

    if (!outFragFile.is_open()) {
        assert(false && "Failed to open output file");
    }
    outFragFile << "#version " << glslVersion << " core\n";

    for (auto& extension : enabledGLSLExtensions) {
        outFragFile << "#extension " << extension << " : require\n";
    }

    outFragFile << merged + CreateFragMain(material.stem());

    outFragFile.close();
    return mergedPaths;
}

const MaterialDefinition* MaterialCooking_t::create(const MaterialCreateInfo &info) {
    const auto matIt = materials.find(info.name);

    if (matIt != materials.end()) {
        return matIt->second;
    }

    std::string materialFile = FileIncludeResolver::openAndGuardIncludes(info.material);
    std::string vertexFile = FileIncludeResolver::openAndGuardIncludes(info.vertex);

    if (materialFile.empty()) {
        assert(false && "Material file not found");
    }
    if (vertexFile.empty()) {
        assert(false && "Vertex file not found");
    }

    const auto it = materialFile.find("material_main(");

    if (it == std::string::npos) {
        std::cerr << "Material main function not found, Expected: " << "SurfaceType material_main(" << info.material.stem() << ")" << std::endl;
        assert(false);
    }

    LargeFilepath path = OutputPath + info.vertex.stem();

    std::fstream outVertexFile((path + "/" + info.vertex.stem() + ".glsl").data(), std::ios::trunc | std::ios::out | std::ios::in);
    outVertexFile << vertexFile;

    path += "/";
    path += info.material.stem();

    std::fstream outMaterialFile((path + "/" + info.material.stem() + ".glsl").data(), std::ios::trunc | std::ios::out | std::ios::in);
    outMaterialFile << materialFile;

    const auto materialClass = materialFile.find(Filepath("struct ") + info.material.stem());

    if (materialClass == std::string::npos) {
        std::cerr << "Material struct not found: " << info.material.stem() << std::endl;
        assert(false);
    }
    std::filesystem::create_directories((path + "/Passes").data());

    auto& matClass = ShaderClass::fromString(shaderCache, materialFile.substr(materialClass));

    const auto vIt = vertices.find(info.vertex);
    const VertexDefinition* vertexDefinition = nullptr;

    if (vIt == vertices.end()) {
        vertexDefinition = new VertexDefinition(info.vertex);
        vertices.emplace(info.vertex, vertexDefinition);
    } else {
        vertexDefinition = vIt->second;
    }

    std::fstream outMaterialDefaultParamsFile((path + "/" + info.material.stem() + ".default").data(), std::ios::trunc | std::ios::out | std::ios::in);

    char* defaultParametersBlock = new char[matClass.size()];

    std::memset(defaultParametersBlock, 0, matClass.size());

    for (auto& [name, defaultValue] : info.defaultParameters.getParameters()) {
        auto member = matClass.findMember(std::string_view(name));

        if (!member) {
            std::cerr << "Default parameter not found: " << name << std::endl;
            assert(false);
        }

        if (defaultValue.size != member->size()) { // TODO change to detect by type
            std::cerr << "Default parameter Type mismatch: " << name << std::endl;
            assert(false);
        }

        std::memcpy(defaultParametersBlock + member->offset, defaultValue.data, member->size());
    }
    outMaterialDefaultParamsFile.write(defaultParametersBlock, static_cast<long long>(matClass.size()));

    auto def = new MaterialDefinition(matClass, vertexDefinition, defaultParametersBlock);
    materials.emplace(info.name, def);
    return def;
}

const MaterialDefinition * MaterialCooking_t::findMaterial(const Filepath &matName) const {
    const auto it = materials.find(matName);
    if (it == materials.end()) {
        return nullptr;
    }
    return it->second;
}

void MaterialCooking_t::importPass(RenderingPassType pass, const Filepath &path) {
    std::string content = FileIncludeResolver::openAndGuardIncludes(path);

    if (content.empty()) {
        assert(false && "Failed to import pass");
    }
    std::filesystem::create_directory((OutputPath + "Passes").data());
    Filepath outputPath = OutputPath + "Passes/" + pass.name() + ".glsl";
    std::fstream out(outputPath.data(), std::ios::trunc | std::ios::out | std::ios::in);

    if (!out.is_open()) {
        assert(false && "Failed to open output file");
    }
    out << content;
}

Filepath MaterialCooking_t::getPassPath(RenderingPassType pass) {
    return OutputPath + "Passes/" + pass.name();
}

const Filepath & MaterialCooking_t::getBasePath() {
    return OutputPath;
}

Filepath MaterialCooking_t::getFilepathForVertex(const Filepath &vertexName) {
    return OutputPath + vertexName.stem() + "/" + vertexName.stem() + ".glsl";
}

MaterialCooking_t MaterialCooking;
