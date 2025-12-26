#include "ZArchetypeComponentRegistry.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryExports.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"

using namespace StateEngine::EngineCore::EntityComponentSystem;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;

EcsEntity ZArchetypeComponentRegistry::CreateEntity() {
	EcsEntity entity;
	if (!freeIndices_->try_dequeue(entity)) {
		entity.entityId = nextEntity_.fetch_add(1, std::memory_order_relaxed);
		entity.generationId = 0;
	}
	UpdateCommand cmd{
		.type = UpdateCommand::CommandType::CreateEntity,
		.entity = entity
	};
	commandsQueue_->enqueue(cmd);
	return entity;
}
void ZArchetypeComponentRegistry::DestroyEntity(EcsEntity entity) {
	UpdateCommand cmd{
		.type = UpdateCommand::CommandType::DestroyEntity,
		.entity = entity
	};
	commandsQueue_->enqueue(cmd);
}

void ZArchetypeComponentRegistry::AddComponentToEntity(EcsEntity entity, size_t componentNameHash, const void* data) {
    const TypeInfo* componentType = nullptr;  // TypeRegistry_GetTypeInfo(componentNameHash);
	if (!componentType) {
		ZLOG_ERROR("ArchetypeEcs") << "TypeInfo not found for component with hash code: " << componentNameHash;
		assert(false && "TypeInfo not found for component");
		return;
	}
	if (componentType->kind != TypeKind::Struct) {
		ZLOG_ERROR("ArchetypeEcs") << "Non structure member " << componentType->Name.c_str() << " cannot be ECS component";
		assert(false && "Component is not structure");
		return;
	}

	auto& allocator = dataCopyAllocators_[writeIndex.load(std::memory_order_relaxed)];
	void* storedData = allocator.allocate(componentType->Size, componentType->Alignment);
	if (componentType->CopyConstructor)
		componentType->CopyConstructor(storedData, data);
	else
		memcpy(storedData, data, componentType->Size);
	
	UpdateCommand cmd{
		 .type = UpdateCommand::CommandType::AddComponent,
		 .entity = entity,
		 .componentHash = componentNameHash,
		 .dataSize = componentType->Size,
		 .dataPtr = storedData
	};
	commandsQueue_->enqueue(cmd);
}
void ZArchetypeComponentRegistry::RemoveComponentFromEntity(EcsEntity entity, size_t componentNameHash) {
	UpdateCommand cmd{
		 .type = UpdateCommand::CommandType::RemoveComponent,
		 .entity = entity,
		 .componentHash = componentNameHash,
	};
	commandsQueue_->enqueue(cmd);
}

void ZArchetypeComponentRegistry::FlushUpdateCommands() {
	uint8_t readIdx = writeIndex.load();
	uint8_t nextWriteIdx = (readIdx + 1) % 2;

	dataCopyAllocators_[nextWriteIdx].clear();
	writeIndex.store(nextWriteIdx);

	constexpr size_t kMaxBulkSize = 64;
	UpdateCommand commands[kMaxBulkSize];
	while (size_t count = commandsQueue_->try_dequeue_bulk(commands, kMaxBulkSize)) {
		for (size_t i = 0; i < count; ++i) {
			auto& curCommand = commands[i];
			if (curCommand.type == UpdateCommand::CommandType::CreateEntity) {
				auto alloc = emptyEntities_->AllocateEntity(curCommand.entity);
				entityIndex_[curCommand.entity] = {
					.archetype = emptyEntities_.get(),
					.chunk = alloc.chunk,
					.rowIndex = alloc.index
				};
			}
			else if (curCommand.type == UpdateCommand::CommandType::DestroyEntity) {
				if (entityIndex_.contains(curCommand.entity)) continue;
				EntityRecord record = entityIndex_[curCommand.entity];
				EcsEntity movedEntity = record.archetype->DestroyEntity(record.chunk, record.rowIndex);
				if (movedEntity != curCommand.entity) {
					entityIndex_[movedEntity] = record;
				}
				entityIndex_.erase(curCommand.entity);
				freeIndices_->enqueue(curCommand.entity);
			}
			else if (curCommand.type == UpdateCommand::CommandType::AddComponent) {
				if (!entityIndex_.contains(curCommand.entity)) continue;

				EntityRecord& oldRecord = entityIndex_[curCommand.entity];
				ZArchetypePool* oldPool = oldRecord.archetype;

				size_t oldHash = oldPool->GetKey();
				size_t newHash = 0;

				if (additionEdgesTransitions_.contains(oldHash) && 
					additionEdgesTransitions_[oldHash].contains(curCommand.componentHash)) 
				{
					newHash = additionEdgesTransitions_[oldHash][curCommand.componentHash];
				}
				else {
					auto& hashes = oldPool->GetComponentHashes();
					ZFixedBuffer<size_t, 32> hashesBuffer;
					for (auto hash : hashes) {
						hashesBuffer.push_back(hash);
					}
					hashesBuffer.push_back(curCommand.componentHash);
					std::sort(hashesBuffer.begin(), hashesBuffer.end());

					auto& newPoolRef = GetOrCreatePool(hashesBuffer);
					newHash = newPoolRef.GetKey();

					additionEdgesTransitions_[oldHash][curCommand.componentHash] = newHash;
				}

				auto& newPool = archetypePools_[newHash];
				MoveEntity(curCommand.entity, oldPool, newPool.get(), curCommand.dataPtr);
			}
			else {
				if (!entityIndex_.contains(curCommand.entity)) continue;
				EntityRecord& oldRecord = entityIndex_[curCommand.entity];

				ZArchetypePool* oldPool = oldRecord.archetype;

				size_t oldHash = oldPool->GetKey();
				size_t newHash = 0;

				if (deletionEdgesTransitions_.contains(oldHash) &&
					deletionEdgesTransitions_[oldHash].contains(curCommand.componentHash))
				{
					newHash = deletionEdgesTransitions_[oldHash][curCommand.componentHash];
				}
				else {
					auto& hashes = oldPool->GetComponentHashes();
					ZFixedBuffer<size_t, 32> hashesBuffer;
					for (auto hash : hashes) {
						if (hash != curCommand.componentHash) hashesBuffer.push_back(hash);
					}
					std::sort(hashesBuffer.begin(), hashesBuffer.end());

					auto& newPoolRef = GetOrCreatePool(hashesBuffer);
					newHash = newPoolRef.GetKey();

					deletionEdgesTransitions_[oldHash][curCommand.componentHash] = newHash;
				}

				auto& newPool = archetypePools_[newHash];
				MoveEntity(curCommand.entity, oldPool, newPool.get(), curCommand.dataPtr);
			}
		}
	}
}

ZArchetypePool& ZArchetypeComponentRegistry::GetOrCreatePool(const ZFixedBuffer<size_t, 32>& componentHashes) {
	size_t newHash = 0;
	for (auto& hash : componentHashes) {
		newHash = Fnv1aHashProvider::CombineHash(newHash, hash);
	}

	if (archetypePools_.contains(newHash))
		return *archetypePools_[newHash].get();

	auto pool = ZUniquePointer<ZArchetypePool>::make(componentHashes);
	for (auto& [queryHash, query] : cachedQueries) {
		bool suitable = true;
		for (auto requiredComponent : query.requiredComponents) {
			if (!pool->HasComponent(requiredComponent)) {
				suitable = false;
				break;
			}
		}
		if (suitable) {
			query.pools.push_back(pool.get());
		}
	}
	archetypePools_[newHash] = std::move(pool);


	return *archetypePools_[newHash].get();
}
void ZArchetypeComponentRegistry::MoveEntity(EcsEntity entity, ZArchetypePool* oldPool, ZArchetypePool* newPool, void* newComponentData) {
	auto newAlloc = newPool->AllocateEntity(entity);
	auto& oldRecord = entityIndex_[entity];

	const auto& newMetaList = newPool->GetComponentsMetadata();


	for (auto& meta : newMetaList) {
		size_t compHash = meta.typeInfo->HashCode;
		void* dstData = newPool->GetComponentData(newAlloc.chunk, newAlloc.index, compHash);
		void* srcData = oldPool->GetComponentData(oldRecord.chunk, oldRecord.rowIndex, compHash);

		void* copySrcData = srcData ? srcData : newComponentData;
		if (meta.typeInfo->MoveConstructor)
			meta.typeInfo->MoveConstructor(dstData, copySrcData);
		else
			memcpy(dstData, copySrcData, meta.typeInfo->Size);

		if (meta.typeInfo->Destructor)
			meta.typeInfo->Destructor(copySrcData);
	}
}

void ZArchetypeComponentRegistry::ForEachComponent(const ForeachCallbackFunction& callback, const ZFixedBuffer<size_t, 32>& requiredComponents) {
	ZFixedBuffer<size_t, 32> requiredComponentsCopy = requiredComponents;
	std::sort(requiredComponentsCopy.begin(), requiredComponentsCopy.end());
	size_t queryHash = 0;
	for (auto& hash : requiredComponentsCopy) {
		queryHash = Fnv1aHashProvider::CombineHash(queryHash, hash);
	}

	if (!cachedQueries.contains(queryHash)) {
		LookupQuery query{
			.requiredComponentsHash = queryHash,
			.requiredComponents = requiredComponents,
		};
		for (auto& [hash, pool] : archetypePools_) {
			bool suitable = true;
			for (auto& requiredComponent : requiredComponents) {
				if (!pool->HasComponent(requiredComponent)) {
					suitable = false;
					break;
				}
			}
			if (!suitable) continue;
			query.pools.push_back(pool.get());
		}
		cachedQueries[queryHash] = std::move(query);
	}

	auto& query = cachedQueries[queryHash];
	ZFixedBuffer<void*, 32> dataPointers;
	for (auto pool : query.pools) {
		ZArchetypePool::ArchetypeChunk* chunk = pool->headChunk_;
		for (size_t i = 0; i < pool->chunksCount && chunk; ++i) {
			dataPointers.clear();
			for (auto& component : requiredComponents) {
				void* data = pool->GetRawComponentArray(chunk, component);
				dataPointers.push_back(data);
			}
			callback(dataPointers.begin(), chunk->count);
			chunk = chunk->next;
		}
	}
}