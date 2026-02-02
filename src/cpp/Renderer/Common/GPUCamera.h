#pragma once
struct GPUCamera {
    glm::mat4 view{};
    glm::mat4 projection{};
    glm::mat4 ortho{};
    glm::mat4 inverseView{};
    glm::mat4 inverseProjection{};
    glm::vec3 cameraPosition{};
    float pad{};
    float nearClip{};
    float farClip{};
    float width{};
    float height{};

    GPUCamera() = default;
};