#include <openGL/Shader/ShaderCompiler.h>

#include "StandardDrawContainer.h"
#include "IndirectDrawContainer.h"
#include "CubemapDrawContainer.h"
#include <Renderer/Graphics/GraphicsContext.h>
#include <Renderer/Resource/Geometry/GeometryComponentType.h>
#include "ScissorDrawContainer.h"
#include "MultiDrawContainer.h"
#include "ComputeCmdContainer.h"
#include <Renderer/Graphics/GLDrawPrimitive.h>

StandardDrawContainer::StandardDrawContainer(GraphicsAllocator *allocator, size_t capCommands): allocator(allocator), capCommands(capCommands) {
    if (capCommands == 0) {
        this->capCommands = 8;
    }
}

void StandardDrawContainer::draw(const DrawCommand &command) {
    if (!commands) {
        commands = allocator->allocate<DrawCommand>(capCommands);
    } else if (numCommands == capCommands) {
        auto newCommands = allocator->allocate<DrawCommand>(capCommands * 2);
        std::memcpy(newCommands, commands, sizeof(DrawCommand) * numCommands);

        commands = newCommands;
        capCommands *= 2;
    }
    commands[numCommands++] = command;
}

void StandardDrawContainer::draw(GraphicsContext &ctx) {
    draw(ctx, commands, numCommands);
}

void StandardDrawContainer::draw(GraphicsContext &ctx, const DrawCommand *commands, const size_t numCommands) {
    for (const auto& cmd : mem::make_range(commands, numCommands)) {
        const GeometryKey geometryKey = cmd.geometry;
        const DrawRange drawRangeKey = cmd.drawRange;

        if (geometryKey == NULL_GEOMETRY_KEY) {
            std::cout << "[ERROR] StandardDrawContainer Geometry: NULL Geometry" << std::endl;
            assert(false);
            continue;
        }

        auto& geometry = *ctx.getGeometry(geometryKey);
        ctx.bindGeometry(geometryKey);

        if (geometry.hasIndices()) {
            glDrawElementsInstancedBaseVertexBaseInstance(
                opengl_enum_cast(cmd.drawPrimitive), drawRangeKey.indexCount,
                geometry.indexType == IndexType::UINT16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                reinterpret_cast<void*>(drawRangeKey.firstIndex * index_type_size(geometry.indexType)),
                drawRangeKey.instanceCount, drawRangeKey.baseVertex, drawRangeKey.firstInstance
            );
        } else {
            glDrawArraysInstancedBaseInstance(opengl_enum_cast(cmd.drawPrimitive), drawRangeKey.baseVertex,
                                              drawRangeKey.indexCount, drawRangeKey.instanceCount,
                                              drawRangeKey.firstInstance
            );
        }
    }
}

ScissorDrawContainer::ScissorDrawContainer(GraphicsAllocator *allocator, size_t capCommands) : allocator(allocator), capCommands(capCommands) {
    if (capCommands == 0) {
        this->capCommands = 8;
    }
}

void ScissorDrawContainer::draw(const ScissorDrawCommand &command) {
    if (!commands) {
        commands = allocator->allocate<ScissorDrawCommand>(capCommands);
    } else if (numCommands == capCommands) {
        auto newCommands = allocator->allocate<ScissorDrawCommand>(capCommands * 2);
        std::memcpy(newCommands, commands, sizeof(ScissorDrawCommand) * numCommands);

        commands = newCommands;
        capCommands *= 2;
    }
    commands[numCommands++] = command;
}

void ScissorDrawContainer::draw(GraphicsContext &ctx) {
    draw(ctx, commands, numCommands);
}

void ScissorDrawContainer::draw(GraphicsContext &ctx, const ScissorDrawCommand *commands, const size_t numCommands) {
    if (!numCommands) return;

    for (const auto& cmd : mem::make_range(commands, numCommands)) {
        ctx.setScissorState(cmd.scissor);
        StandardDrawContainer::draw(ctx, &cmd, 1);
    }
    ctx.setScissorState(ScissorState{});
}

IndirectDrawContainer::IndirectDrawContainer(GraphicsAllocator *allocator, size_t capCommands): allocator(allocator), capCommands(capCommands) {
    if (capCommands == 0) {
        this->capCommands = 8;
    }
}

void IndirectDrawContainer::draw(const IndirectDrawCommand &command) {
    if (!commands) {
        commands = allocator->allocate<IndirectDrawCommand>(capCommands);
    } else if (numCommands == capCommands) {
        auto newCommands = allocator->allocate<IndirectDrawCommand>(capCommands * 2);
        std::memcpy(newCommands, commands, sizeof(IndirectDrawCommand) * numCommands);

        commands = newCommands;

        capCommands *= 2;
    }
    commands[numCommands++] = command;
}

void IndirectDrawContainer::draw(GraphicsContext &ctx) {
    for (const auto& cmd : mem::make_range(commands, numCommands)) {
        const GeometryKey geometryKey = cmd.geometry;
        const DrawIndirectKey drawRangeKey = cmd.indirect;

        const Geometry* geometry = ctx.getGeometry(geometryKey);
        ctx.bindGeometry(geometryKey);
        ctx.bindIndirectDrawBuffer(drawRangeKey.buffer);

        size_t idbOffset = drawRangeKey.offset + drawRangeKey.buffer.getOffset();

        if (geometry->hasIndices()) {
            glMultiDrawElementsIndirect(
                opengl_enum_cast(cmd.drawPrimitive),
                geometry->indexType == IndexType::UINT16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                reinterpret_cast<void*>(idbOffset * sizeof(DrawElementsIndirectCommand)),
                drawRangeKey.count,
                drawRangeKey.stride
            );
        } else {
            assert(false);
            glMultiDrawArraysIndirect(
                opengl_enum_cast(cmd.drawPrimitive),
                reinterpret_cast<void*>(idbOffset),
                drawRangeKey.count,
                drawRangeKey.stride
            );
        }
    }
}

void StandardElementsIndirectMultiDrawContainer::draw(const DrawCommand &cmd) {
    if (nextIndirectIdx >= mappedBuffer.size()) {
        assert(false);
        return;
    }
    auto& indirect = mappedBuffer[nextIndirectIdx];

    indirect.baseInstance = cmd.drawRange.firstInstance;
    indirect.baseVertex = cmd.drawRange.baseVertex;
    indirect.firstIndex = cmd.drawRange.firstIndex;
    indirect.indexCount = cmd.drawRange.indexCount;
    indirect.instanceCount = cmd.drawRange.instanceCount;

    GraphicsVectorView rangesVector(allocator, ranges, numRanges, capRanges);

    if (rangesVector.empty()) {
        rangesVector.emplace_back(cmd.geometry, cmd.drawPrimitive, nextIndirectIdx, 1);
    } else {
        auto& back = rangesVector.back();

        if (back.geometry == cmd.geometry && back.primitive == cmd.drawPrimitive) {
            ++back.count;
        } else {
            rangesVector.emplace_back(cmd.geometry, cmd.drawPrimitive, nextIndirectIdx, 1);
        }
    }
    ++nextIndirectIdx;
}

void StandardElementsIndirectMultiDrawContainer::draw(GraphicsContext &ctx) {
    ctx.bindIndirectDrawBuffer(indirectBuffer);

    for (const auto& range : mem::make_range(ranges, numRanges)) {
        const GeometryKey geometryKey = range.geometry;

        const Geometry* geometry = ctx.getGeometry(geometryKey);

        assert(geometry->hasIndices());

        ctx.bindGeometry(geometryKey);

        const size_t byteOffset = indirectBuffer.getOffset() + range.offset * sizeof(DrawElementsIndirectCommand);

        glMultiDrawElementsIndirect(
            opengl_enum_cast(range.primitive),
            geometry->indexType == IndexType::UINT16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
            reinterpret_cast<void*>(byteOffset),
            range.count,
            0
        );
    }
}

void CubemapDrawContainer::draw(GraphicsContext &ctx) {
    for (auto& face : faceContainers) {
        if (!face) continue;

        face->draw(ctx);
    }
}

void MultiDrawContainer::addDrawContainer(GraphicsDrawContainer *pass) {
    if (capDrawContainers == 0) {
        capDrawContainers = 2;
        drawContainers = allocator->allocate<GraphicsDrawContainer*>(capDrawContainers);
    } else if (numDrawContainers == capDrawContainers) {
        auto newCommands = allocator->allocate<GraphicsDrawContainer*>(capDrawContainers * 2);
        std::memcpy(newCommands, drawContainers, sizeof(GraphicsDrawContainer*) * numDrawContainers);

        drawContainers = newCommands;

        capDrawContainers *= 2;
    }
    drawContainers[numDrawContainers++] = pass;
}

void MultiDrawContainer::draw(GraphicsContext &ctx) {
    for (const auto cmd : mem::make_range(drawContainers, numDrawContainers)) {
        cmd->draw(ctx);
    }
}

ComputeCmdContainer::ComputeCmdContainer(GraphicsAllocator *allocator, size_t capCommands) : allocator(allocator), capCommands(capCommands) {
    if (capCommands == 0) {
        capCommands = 2;
    }
    commands = allocator->allocate<ComputeCommand>(capCommands);
}

void ComputeCmdContainer::dispatch(const ComputeCommand &command) {
    if (!commands) {
        commands = allocator->allocate<ComputeCommand>(capCommands);
    } else if (numCommands == capCommands) {
        auto newCommands = allocator->allocate<ComputeCommand>(capCommands * 2);
        std::memcpy(newCommands, commands, sizeof(ComputeCommand) * numCommands);

        commands = newCommands;

        capCommands *= 2;
    }
    commands[numCommands++] = command;
}

void ComputeCmdContainer::dispatch(int x, int y, int z) {
    dispatch(ComputeCommand{{x, y, z}});
}

void ComputeCmdContainer::draw(GraphicsContext &ctx) {
    const glm::ivec3 threads = ctx.getCurrentShaderProgram()->threads();

    for (auto& command : mem::make_range(commands, numCommands)) {
        const auto& [invocations] = command;

        const int num_groups_x = (invocations.x + threads.x - 1) / threads.x;
        const int num_groups_y = (invocations.y + threads.y - 1) / threads.y;
        const int num_groups_z = (invocations.z + threads.z - 1) / threads.z;

        glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    }
}

IndirectComputeCmdContainer::IndirectComputeCmdContainer(GraphicsAllocator *allocator, size_t capCommands) {
    if (capCommands == 0) {
        capCommands = 1;
    }
    commands = allocator->allocate<IndirectComputeCommand>(capCommands);
}

void IndirectComputeCmdContainer::dispatch(const IndirectComputeCommand &command) {
    if (!commands) {
        commands = allocator->allocate<IndirectComputeCommand>(capCommands);
    } else if (numCommands == capCommands) {
        auto newCommands = allocator->allocate<IndirectComputeCommand>(capCommands * 2);
        std::memcpy(newCommands, commands, sizeof(IndirectComputeCommand) * numCommands);

        commands = newCommands;

        capCommands *= 2;
    }
    commands[numCommands++] = command;
}

void IndirectComputeCmdContainer::draw(GraphicsContext &ctx) {
    for (auto& [indirect, offset] : mem::make_range(commands, numCommands)) {
        ctx.bindIndirectComputeBuffer(indirect);
        glDispatchComputeIndirect(offset);
    }
}
