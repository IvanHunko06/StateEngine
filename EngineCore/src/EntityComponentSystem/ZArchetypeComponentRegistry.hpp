#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZFixedBuffer.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"
#include "EngineCore/EntityComponentSystem/ForeachCallbackFunction.hpp"
#include "EngineCore/EntityComponentSystem/IEcsComponentRegistry.hpp"
#include "EngineCore/MemoryManagment/ZFrameAllocator.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"
#include "ZArchetypePool.hpp"

namespace StateEngine::EngineCore::EntityComponentSystem {
    class ZArchetypeComponentRegistry : public IEcsDebugComponentRegistry {
        struct alignas(32) EntityRecord {
            ZArchetypePool* archetype;
            ZArchetypePool::ArchetypeChunk* chunk;
            size_t rowIndex;
            size_t generationId;
        };
        struct UpdateCommand {
            enum class CommandType { CreateEntity, AddComponent, RemoveComponent, DestroyEntity };
            CommandType type;
            uint32_t dataSize;    
            EcsEntity entity;
            size_t componentHash;   
            void* dataPtr;
        };
        struct LookupQuery {
            size_t requiredComponentsHash;
            ZFixedBuffer<size_t, 32> requiredComponents;
            ZFixedBuffer<ZArchetypePool*, 32> pools;
        };

      private:
        DataStructures::ZBuffer<EntityRecord> entityIndex_ {};

        static constexpr size_t kMaxBulkSize = 64;  // Константа для буфера
        DataStructures::ZBuffer<UpdateCommand> commandBuffer_;

        DataStructures::ZHashMap<size_t, SmartPointers::ZUniquePointer<ZArchetypePool>> archetypePools_ {};
        DataStructures::ZHashMap<size_t, DataStructures::ZHashMap<size_t, size_t>> additionEdgesTransitions_ {};
        DataStructures::ZHashMap<size_t, DataStructures::ZHashMap<size_t, size_t>> deletionEdgesTransitions_ {};

        DataStructures::ZHashMap<size_t, LookupQuery> cachedQueries_;
        SmartPointers::ZUniquePointer<ZArchetypePool> emptyEntities_ =
            SmartPointers::ZUniquePointer<ZArchetypePool>::make(ZFixedBuffer<size_t, 32>());
        std::atomic<uint32_t> nextEntity_ = 1;
        SmartPointers::ZUniquePointer<Threading::MPMCQueue<UpdateCommand>> commandsQueue_ =
            SmartPointers::ZUniquePointer<MPMCQueue<UpdateCommand>>::make();
        SmartPointers::ZUniquePointer<Threading::MPMCQueue<EcsEntity>> freeIndices_ =
            SmartPointers::ZUniquePointer<MPMCQueue<EcsEntity>>::make();

        static constexpr size_t kFrameAllocatorSize = 5 * 1024 * 1024;
        std::array<MemoryManagment::ZFrameAllocator, 2> dataCopyAllocators_ {
            {MemoryManagment::ZFrameAllocator(kFrameAllocatorSize),
             MemoryManagment::ZFrameAllocator(kFrameAllocatorSize)}};
        std::atomic<uint8_t> writeIndex {0};

      public:
        ZArchetypeComponentRegistry()
        {
            commandBuffer_.resize(kMaxBulkSize);
        }

        EcsEntity CreateEntity();
        void DestroyEntity(EcsEntity entity);

        void AddComponent(EcsEntity entity, size_t componentNameHash, const void* data);
        void RemoveComponent(EcsEntity entity, size_t componentNameHash);

        void ForEachComponent(const ForeachCallbackFunction& callback,
                              const ZFixedBuffer<size_t, 32>& requiredComponents);
        void* GetComponent(EcsEntity entity, size_t componentType);

        void FlushUpdateCommands() noexcept;

      private:
        ZArchetypePool& GetOrCreatePool(const DataStructures::ZFixedBuffer<size_t, 32>& componentHashes);
        void MoveEntity(EcsEntity entity, ZArchetypePool* oldPool, ZArchetypePool* newPool, void* newComponentData);

        void CreateEntityInternal(EcsEntity entity);
        void RemoveEntityInternal(EcsEntity entity);
        void RemoveEntityFromPool(EcsEntity entity);
        void AddComponentInternal(EcsEntity entity, size_t componentNameHash, void* data);
        void RemoveComponentInternal(EcsEntity entity, size_t componentNameHash);

        inline EntityRecord* GetEntityRecord(EcsEntity entity)
        {
            if (entity.entityId >= entityIndex_.size())
                return nullptr;
            EntityRecord& record = entityIndex_[entity.entityId];
            if (record.generationId != entity.generationId || record.archetype == nullptr)
                return nullptr;
            return &record;
        }
    };
}  // namespace StateEngine::EngineCore::EntityComponentSystem