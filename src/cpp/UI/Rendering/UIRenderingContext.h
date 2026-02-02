#pragma once
#include <ECS/Component/Component.h>
#include <ECS/Component/ComponentTypeRegistry.h>
#include <ECS/Component/SingletonTypeRegistry.h>
#include <memory/byte_arena.h>
#include <UI/UI.h>
#include <UI/Rendering/UIStyleType.h>

namespace ui {
    class UIRenderingContext {
        std::vector<UIDrawCommand> drawCommands{};
        mem::byte_arena<> arena = mem::create_byte_arena(4 * 1024);
    public:
        template <typename TStyle>
        void draw(const UIStyleDrawCommand<TStyle>& cmd) {
            UIDrawCommand abCmd;
            abCmd.styleData = new (arena.allocate<TStyle>(1)) TStyle(cmd.style);
            abCmd.transform = cmd.transform;

            abCmd.styleType = UIStyleTypes.registerType<TStyle>();

            drawCommands.emplace_back(abCmd);
        }

        auto& getDrawCommands() {
            return drawCommands;
        }

        void reset() {
            arena.reset_compact();
            drawCommands.clear();
        }

        template <typename T>
        T* create(auto&&... args) {
            return new (arena.allocate<T>(1)) T(std::forward<decltype(args)>(args)...);
        }
    };
}
