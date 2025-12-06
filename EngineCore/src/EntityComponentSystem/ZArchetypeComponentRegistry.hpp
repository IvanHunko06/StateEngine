#pragma once
#include "ZArchetypePool.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"
#include "EngineCore/MemoryManagment/ZFrameAllocator.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/EntityComponentSystem/ForeachCallbackFunction.hpp"

using StateEngine::EngineCore::DataStructures::ZHashMap;
using StateEngine::EngineCore::Threading::MPMCQueue;
using StateEngine::EngineCore::MemoryManagment::ZFrameAllocator;
using StateEngine::EngineCore::SmartPointers::ZUniquePointer;
using StateEngine::EngineCore::DataStructures::ZFunction;
namespace StateEngine::EngineCore::EntityComponentSystem {
	class ZArchetypeComponentRegistry {
		struct EntityRecord {
			ZArchetypePool* archetype;
			ZArchetypePool::ArchetypeChunk* chunk;
			size_t rowIndex;
		};
		struct UpdateCommand {
			enum class CommandType {
				CreateEntity,
				AddComponent,
				RemoveComponent,
				DestroyEntity
			};
			CommandType type;
			EcsEntity entity;
			size_t componentHash;
			size_t dataSize;
			void* dataPtr;
		};
	private:
		static constexpr size_t kFrameAllocatorSize = 500 * 1024;
		ZHashMap<EcsEntity, EntityRecord> entityIndex_{};
		ZHashMap<size_t, ZUniquePointer<ZArchetypePool>> archetypePools_{};
		ZHashMap <size_t, ZHashMap<size_t, size_t>> additionEdgesTransitions_{};
		ZHashMap <size_t, ZHashMap<size_t, size_t>> deletionEdgesTransitions_{};
		ZArchetypePool emptyEntities_{ {} };
		EcsEntity nextEntity_ = 1;
		MPMCQueue<UpdateCommand> commandsQueue_{};
		MPMCQueue<EcsEntity> freeIndices_{};
		std::array<ZFrameAllocator, 2> dataCopyAllocators_{ {
				ZFrameAllocator(kFrameAllocatorSize),
				ZFrameAllocator(kFrameAllocatorSize)
		} };
		std::atomic<uint8_t> writeIndex{ 0 };
	public:
		EcsEntity CreateEntity();
		void DestroyEntity(EcsEntity entity);

		void AddComponentToEntity(EcsEntity entity, size_t componentNameHash, const void* data);
		void RemoveComponentFromEntity(EcsEntity entity, size_t componentNameHash);

		void ForEachEntity(ForeachCallbackFunction&& callback, ZBuffer<size_t> requiredComponents);

		void FlushUpdateCommands();
	private:
		ZArchetypePool& GetOrCreatePool(const ZBuffer<size_t>& componentHashes);
		void MoveEntity(EcsEntity entity, ZArchetypePool* oldPool, ZArchetypePool* newPool, void* newComponentData);
	};
}