#pragma once

class RenderPassContainer {
    IRenderPass** graphicPasses{};
    size_t numPasses{};
    size_t capPasses{};
public:
    RenderPassContainer() = default;

    template <typename Pass, typename... Args>
    requires std::is_base_of_v<IRenderPass, Pass>
    Pass& createPass(Renderer* renderer, GraphicsAllocator* allocator, const std::string_view name, Args&&... args) {
        auto mem = static_cast<char *>(allocator->allocate(name.length() + 1, 1));
        std::memcpy(mem, name.data(), name.length());
        mem[name.length()] = '\0';

        if constexpr (std::is_same_v<Pass, GraphicsPass>) {
            auto* pass = (Pass*)allocator->allocate(sizeof(Pass), alignof(Pass));

            new (pass) Pass(std::string_view(mem, name.length()), allocator, renderer->getShaderProgram(args...));

            mem::emplace_back<IRenderPass*>(*allocator, graphicPasses,
                numPasses, capPasses,
                capPasses * 2, pass
            );
            return *pass;
        }
    }
};
