#pragma once
#include "EcsEntity.hpp"
#include "EngineCore/DataStructures/ZFixedBuffer.hpp"
#include "ForeachCallbackFunction.hpp"

namespace StateEngine::EngineCore::EntityComponentSystem {
    class IEcsComponentRegistry {
      public:
        using ForEeachComponentsBuffer = DataStructures::ZFixedBuffer<size_t, 32>;

        virtual EcsEntity CreateEntity()                                                             = 0;
        virtual void DestroyEntity(EcsEntity entity)                                                 = 0;
        virtual void AddComponent(EcsEntity entity, size_t componentType, const void* componentData) = 0;
        virtual void RemoveComponent(EcsEntity entity, size_t componentType)                         = 0;
        virtual void ForEachComponent(const ForeachCallbackFunction& callback,
                                      const ForEeachComponentsBuffer& components)                    = 0;
        virtual void* GetComponent(EcsEntity entity, size_t componentType)                           = 0;
    };

    class IEcsDebugComponentRegistry : public IEcsComponentRegistry {
      public:
        /// <summary>
        /// Commits all pending changes. Not thread-safe. Not recommended for manual use except for testing.
        /// </summary>
        virtual void FlushUpdateCommands() noexcept = 0;
    };
}  // namespace StateEngine::EngineCore::EntityComponentSystem