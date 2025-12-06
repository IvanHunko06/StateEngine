#pragma once
#include <cstdint>
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"

using StateEngine::EngineCore::DataStructures::ZBuffer;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;
using StateEngine::EngineCore::DataStructures::ZHashMap;
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
			size_t size{ 0 };
			size_t alignment{ 0 };
			size_t chunkOffset{ 0 };
			const TypeInfo* typeInfo{ nullptr };
		};
	private:
		ArchetypeChunk* headChunk_{ nullptr };
		uint32_t chunkCapacity_{ 0 };
		size_t entityIdsOffset_{ 0 };
		size_t chunkSize_{ 0 };
		size_t archetypeKey{ 0 };
		size_t chunksCount{ 0 };
		ZBuffer<size_t> componentNamesHashCodes_{};
		ZBuffer<ComponentMetadata> components_{};
		ZHashMap<size_t, size_t> componentHashToIndexMap_{};
		friend class ZArchetypeComponentRegistry;
	public:
		ZArchetypePool(const ZBuffer<size_t>& components);
		~ZArchetypePool();
		ZArchetypePool() = default;
		struct AllocResult {
			ArchetypeChunk* chunk;
			uint32_t index;
		};
		AllocResult AllocateEntity(EcsEntity entityID);
		EcsEntity DestroyEntity(ArchetypeChunk* chunk, uint32_t index);
		void* GetComponentData(ArchetypeChunk* chunk, uint32_t index, size_t componentHash);
		inline size_t GetKey() const noexcept {
			return archetypeKey;
		}
		inline const ZBuffer<size_t> GetComponentHashes() const noexcept {
			return componentNamesHashCodes_;
		}
		inline const ZBuffer<ComponentMetadata> GetComponentsMetadata() const noexcept {
			return components_;
		}
		inline const EcsEntity* GetEntityIdArray(ArchetypeChunk* chunk) const noexcept{
			return reinterpret_cast<EcsEntity*>(reinterpret_cast<uint8_t*>(chunk) + entityIdsOffset_);
		}
		inline const ArchetypeChunk* GetHeadChunk() const noexcept{ return headChunk_; }
		inline void* GetRawComponentArray(ArchetypeChunk* chunk, size_t componentHash) {
			if (!componentHashToIndexMap_.contains(componentHash)) return nullptr;

			size_t idx = componentHashToIndexMap_[componentHash];
			auto& meta = components_[idx];
			return reinterpret_cast<uint8_t*>(chunk) + meta.chunkOffset;
		}
	private:
		void CalculateLayout(); // Вызовем это в конструкторе
		ArchetypeChunk* AllocateChunk(); // Создание нового блока памяти
		inline size_t AlignUp(size_t value, size_t alignment) {
			return (value + (alignment - 1)) & ~(alignment - 1);
		}
	};
}