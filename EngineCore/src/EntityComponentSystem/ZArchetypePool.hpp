#pragma once
#include "EngineCore/DataStructures/ZFixedBuffer.hpp"
#include "EngineCore/DataStructures/ZFixedHashMap.hpp"
#include "EngineCore/DataStructures/ZFixedHashSet.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"
#include <cstdint>

using StateEngine::EngineCore::DataStructures::ZFixedBuffer;
using StateEngine::EngineCore::DataStructures::ZFixedHashMap;
using StateEngine::EngineCore::DataStructures::ZFixedHashSet;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;
namespace StateEngine::EngineCore::EntityComponentSystem {
    class ZArchetypeComponentRegistry;
    class ZArchetypePool {
      private:
        struct ArchetypeChunk {
            ArchetypeChunk* next;
            uint32_t count;
            uint32_t index;
        };
        struct ComponentMetadata {
            size_t size {0};
            size_t alignment {0};
            size_t chunkOffset {0};
            const TypeInfo* typeInfo {nullptr};
        };

      private:
        ArchetypeChunk* headChunk_ {nullptr};
        uint32_t chunkCapacity_ {0};
        size_t entityIdsOffset_ {0};
        size_t chunkSize_ {0};
        size_t archetypeKey {0};
        size_t chunksCount {0};
        ZFixedHashSet<size_t, 32> componentNamesHashCodes_ {};
        ZFixedBuffer<ComponentMetadata, 32> components_ {};
        ZFixedHashMap<size_t, size_t, 32> componentHashToIndexMap_ {};
        friend class ZArchetypeComponentRegistry;

      public:
        ZArchetypePool(const ZFixedBuffer<size_t, 32>& components);
        ~ZArchetypePool();
        ZArchetypePool() = default;
        struct AllocResult {
            ArchetypeChunk* chunk;
            uint32_t index;
        };
        AllocResult AllocateEntity(EcsEntity entityID);
        EcsEntity DestroyEntity(ArchetypeChunk* chunk, uint32_t index);
        void* GetComponentData(ArchetypeChunk* chunk, uint32_t index, const ComponentMetadata& meta) const noexcept{
            auto* buffer = reinterpret_cast<uint8_t*>(chunk);
            return buffer + meta.chunkOffset + (index * meta.size);
        }
        const ComponentMetadata* GetMetadata(size_t componentHash) const noexcept {
            for (const auto& meta : components_) {
                if (meta.typeInfo->HashCode == componentHash) {
                    return &meta;
                }
            }
            return nullptr;
        }
        inline size_t GetKey() const noexcept
        {
            return archetypeKey;
        }
        inline const ZFixedHashSet<size_t, 32>& GetComponentHashes() const noexcept
        {
            return componentNamesHashCodes_;
        }
        inline const ZFixedBuffer<ComponentMetadata, 32>& GetComponentsMetadata() const noexcept
        {
            return components_;
        }
        inline const EcsEntity* GetEntityIdArray(ArchetypeChunk* chunk) const noexcept
        {
            return reinterpret_cast<EcsEntity*>(reinterpret_cast<uint8_t*>(chunk) + entityIdsOffset_);
        }
        inline const ArchetypeChunk* GetHeadChunk() const noexcept
        {
            return headChunk_;
        }
        inline void* GetRawComponentArray(ArchetypeChunk* chunk, size_t componentHash)
        {
            auto it = componentHashToIndexMap_.find(componentHash);
            if (it == componentHashToIndexMap_.end()) {
                return nullptr;
            }

            size_t idx = it->second;
            auto& meta = components_[idx];
            return reinterpret_cast<uint8_t*>(chunk) + meta.chunkOffset;
        }
        inline bool HasComponent(size_t componentHash)
        {
            return componentNamesHashCodes_.contains(componentHash);
        }

      private:
        void CalculateLayout();
        ArchetypeChunk* AllocateChunk();
        inline size_t AlignUp(size_t value, size_t alignment)
        {
            return (value + (alignment - 1)) & ~(alignment - 1);
        }
    };
}  // namespace StateEngine::EngineCore::EntityComponentSystem