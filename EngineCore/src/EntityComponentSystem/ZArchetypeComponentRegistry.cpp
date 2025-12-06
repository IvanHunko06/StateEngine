#include "ZArchetypeComponentRegistry.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryExports.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/EngineTypeSystem/TypeKind.hpp"
using namespace StateEngine::EngineCore::EntityComponentSystem;
using namespace StateEngine::EngineCore::EngineTypeSystem;

EcsEntity ZArchetypeComponentRegistry::CreateEntity() {
	EcsEntity entity = nextEntity_++;
	UpdateCommand cmd{
		.type = UpdateCommand::CommandType::CreateEntity,
		.entity = entity
	};
	commandsQueue_.enqueue(cmd);
	return entity;
}
void ZArchetypeComponentRegistry::DestroyEntity(EcsEntity entity) {
	UpdateCommand cmd{
		.type = UpdateCommand::CommandType::DestroyEntity,
		.entity = entity
	};
	commandsQueue_.enqueue(cmd);
}

void ZArchetypeComponentRegistry::AddComponentToEntity(EcsEntity entity, size_t componentNameHash, const void* data) {
	const TypeInfo* componentType = TypeRegistry_GetTypeInfo(componentNameHash);
	if (!componentType) {
		ZLOG_ERROR("ArchetypeEcs") << "TypeInfo not found for component with hash code: " << componentNameHash;
		assert(false && "TypeInfo not found for component");
		return;
	}
	if (componentType->kind != TypeKind::Struct) {
		ZLOG_ERROR("ArchetypeEcs") << "Non structure member " << componentType->name.c_str() << " cannot be ECS component";
		assert(false && "Component is not structure");
		return;
	}

	auto& allocator = dataCopyAllocators_[writeIndex.load(std::memory_order_relaxed)];
	void* storedData = allocator.allocate(componentType->size, componentType->alignment);
	if (componentType->copyConstructor)
		componentType->copyConstructor(storedData, data);
	else
		memcpy(storedData, data, componentType->size);
	
	UpdateCommand cmd{
		 .type = UpdateCommand::CommandType::AddComponent,
		 .entity = entity,
		 .componentHash = componentNameHash,
		 .dataSize = componentType->size,
		 .dataPtr = storedData
	};
	commandsQueue_.enqueue(cmd);
}
void ZArchetypeComponentRegistry::RemoveComponentFromEntity(EcsEntity entity, size_t componentNameHash) {
	UpdateCommand cmd{
		 .type = UpdateCommand::CommandType::RemoveComponent,
		 .entity = entity,
		 .componentHash = componentNameHash,
	};
	commandsQueue_.enqueue(cmd);
}

void ZArchetypeComponentRegistry::FlushUpdateCommands() {
	uint8_t readIdx = writeIndex.load();
	uint8_t nextWriteIdx = (readIdx + 1) % 2;

	dataCopyAllocators_[nextWriteIdx].clear();
	writeIndex.store(nextWriteIdx);

	constexpr size_t kMaxBulkSize = 64;
	UpdateCommand commands[kMaxBulkSize];
	while (size_t count = commandsQueue_.try_dequeue_bulk(commands, kMaxBulkSize)) {
		for (size_t i = 0; i < count; ++i) {
			auto& curCommand = commands[i];
			if (curCommand.type == UpdateCommand::CommandType::CreateEntity) {
				auto alloc = emptyEntities_.AllocateEntity(curCommand.entity);
				entityIndex_[curCommand.entity] = {
					.archetype = &emptyEntities_,
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
					ZBuffer<size_t> hashes = oldPool->GetComponentHashes();
					hashes.push_back(curCommand.componentHash);
					std::sort(hashes.begin(), hashes.end());

					auto& newPoolRef = GetOrCreatePool(hashes);
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
					ZBuffer<size_t> hashes = oldPool->GetComponentHashes();
					hashes.erase(std::remove(hashes.begin(), hashes.end(), curCommand.componentHash), hashes.end());
					std::sort(hashes.begin(), hashes.end());

					auto& newPoolRef = GetOrCreatePool(hashes);
					newHash = newPoolRef.GetKey();

					deletionEdgesTransitions_[oldHash][curCommand.componentHash] = newHash;
				}

				auto& newPool = archetypePools_[newHash];
				MoveEntity(curCommand.entity, oldPool, newPool.get(), curCommand.dataPtr);
			}
		}
	}

}

ZArchetypePool& ZArchetypeComponentRegistry::GetOrCreatePool(const ZBuffer<size_t>& componentHashes) {
	size_t newHash = 0;
	for (auto& hash : componentHashes) {
		newHash = Fnv1aHashProvider::combineHash(newHash, hash);
	}

	if (archetypePools_.contains(newHash))
		return *archetypePools_[newHash].get();

	auto pool = ZUniquePointer<ZArchetypePool>::make(componentHashes);
	archetypePools_[newHash] = std::move(pool);

	return *archetypePools_[newHash].get();
}
void ZArchetypeComponentRegistry::MoveEntity(EcsEntity entity, ZArchetypePool* oldPool, ZArchetypePool* newPool, void* newComponentData) {
	auto newAlloc = newPool->AllocateEntity(entity);
	auto& oldRecord = entityIndex_[entity];

	const auto& newMetaList = newPool->GetComponentsMetadata();


	for (auto& meta : newMetaList) {
		size_t compHash = meta.typeInfo->hashCode;
		void* dstData = newPool->GetComponentData(newAlloc.chunk, newAlloc.index, compHash);
		void* srcData = oldPool->GetComponentData(oldRecord.chunk, oldRecord.rowIndex, compHash);

		void* copySrcData = srcData ? srcData : newComponentData;
		if (meta.typeInfo->moveConstructor)
			meta.typeInfo->moveConstructor(dstData, copySrcData);
		else
			memcpy(dstData, copySrcData, meta.typeInfo->size);

		if (meta.typeInfo->destructor)
			meta.typeInfo->destructor(copySrcData);
	}
}

void ZArchetypeComponentRegistry::ForEachEntity(ForeachCallbackFunction&& callback, ZBuffer<size_t> requiredComponents) {

}