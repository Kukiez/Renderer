#pragma once

#include <glm/vec4.hpp>
#include <sstream>
#include <filesystem>
#include <string>
#include <glm/mat4x4.hpp>

namespace util {
    inline glm::vec4 hex_to_vec4(const uint32_t hex) {
        auto r = static_cast<float>((hex >> 24) & 0xFF);
        float g = static_cast<float>((hex >> 16) & 0xFF);
        float b = static_cast<float>((hex >> 8)  & 0xFF);
        float a = static_cast<float>((hex >> 0)  & 0xFF);

        r /= 255.0f;
        g /= 255.0f;
        b /= 255.0f;
        a /= 255.0f;

        return {r,g,b,a};
    }

    inline std::vector<std::string> getFolders(const std::string& path) {
        std::vector<std::string> folders;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                folders.push_back(entry.path().filename().string());
            }
        }
        return folders;
    }

    inline std::vector<std::string> getFiles(const std::string& path) {
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
        return files;
    }

    template <typename T>
    constexpr glm::mat4 assimpToGLM(const T& from) {
		glm::mat4 to;

		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
    }

    inline std::string GetDirFromFilename(const std::string& Filename) {
        std::string::size_type SlashIndex;

        SlashIndex = Filename.find_last_of("\\");

        if (SlashIndex == -1) {
            SlashIndex = Filename.find_last_of("/");
        }

        std::string Dir;

        if (SlashIndex == std::string::npos) {
            Dir = ".";
        }
        else if (SlashIndex == 0) {
            Dir = "/";
        }
        else {
            Dir = Filename.substr(0, SlashIndex);
        }

        return Dir;
    }
}
