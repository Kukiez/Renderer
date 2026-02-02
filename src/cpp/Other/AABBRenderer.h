#pragma once
#include <glm/ext/matrix_transform.hpp>
#include <Math/Shapes/AABB.h>
#include "../Renderer/RenderingStages/LoadPass.h"
#include "../Renderer/RenderingStages/OpaquePass.h"
#include "../Renderer/Scene/Primitives/IPrimitive.h"
#include <Renderer/Scene/Primitives/Primitive.h>

struct DrawAABBCommand {
    glm::mat4 model{};
    glm::vec4 color{};

    static DrawAABBCommand fromAABB(const AABB& aabb, glm::vec3 color) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, aabb.center);
        model = glm::scale(model, aabb.halfSize);
        return DrawAABBCommand{model, glm::vec4(color, 1.f)};
    }

    DrawAABBCommand() = default;

    DrawAABBCommand(const glm::mat4 & mat, const glm::vec4 vec)
        : model(mat), color(vec) {}
};

struct DrawCubeCommand : DrawAABBCommand {
    using DrawAABBCommand::DrawAABBCommand;

    static DrawCubeCommand fromAABB(const AABB& aabb, const glm::vec3 color) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, aabb.center);
        model = glm::scale(model, aabb.halfSize);
        return DrawCubeCommand{model, glm::vec4(color, 1.f)};
    }

    static DrawCubeCommand fromAABB(const AABB& aabb, const glm::vec4 color) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, aabb.center);
        model = glm::scale(model, aabb.halfSize);
        return DrawCubeCommand{model, color};
    }
};

struct BoxCollection : IPrimitiveCollection {
    AABB aabb;
    glm::vec4 color;

    BoxCollection(const AABB& aabb, glm::vec4 color) : aabb(aabb), color(color) {}
};

struct AABBRenderer : SystemComponent {
    ShaderKey aabbShader;
    ShaderKey aabbTransparentShader;
    GeometryKey cubeGeometry;
    GeometryKey aabbGeometry;

    void onLoad(RendererLoadView level);
};

struct SimpleShapePipeline : IPrimitivePipeline<BoxCollection> {
    ShaderKey shader;
    GeometryKey geometry;

    SimpleShapePipeline(ShaderKey shader, GeometryKey geometry) : shader(shader), geometry(geometry) {}

    void onRender(const GraphicsPassInvocation& invocation) const;
};