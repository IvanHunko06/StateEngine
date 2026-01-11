#include "ZArchetypeComponentRegistry.hpp"
#include "ZArchetypePool.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"
#include "EngineCore/EntityComponentSystem/ForeachCallbackFunction.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include <ranges>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

using namespace StateEngine::EngineCore::EntityComponentSystem;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;
using namespace StateEngine::EngineCore::SmartPointers;

EcsEntity ZArchetypeComponentRegistry::CreateEntity()
{
    EcsEntity entity;
    if (!freeIndices_->try_dequeue(entity)) {
        entity.entityId     = nextEntity_.fetch_add(1, std::memory_order_relaxed);
        entity.generationId = 0;
    }
    const UpdateCommand cmd {.type = UpdateCommand::CommandType::CreateEntity, .entity = entity};
    commandsQueue_->enqueue(cmd);
    return entity;
}

void ZArchetypeComponentRegistry::DestroyEntity(EcsEntity entity)
{
    const UpdateCommand cmd {.type = UpdateCommand::CommandType::DestroyEntity, .entity = entity};
    commandsQueue_->enqueue(cmd);
}

void ZArchetypeComponentRegistry::AddComponent(EcsEntity entity, size_t componentNameHash, const void* data)
{
    const TypeInfo& componentType = TypeRegistry::GetRequiredType(componentNameHash);

    if (componentType.kind != TypeKind::Struct) {
        ZLOG_ERROR("ArchetypeEcs") << "Non structure type " << componentType.Name.c_str() << " cannot be ECS component";
        assert(false && "Component is not structure");
        return;
    }

    auto& allocator  = dataCopyAllocators_[writeIndex.load(std::memory_order_relaxed)];
    void* storedData = allocator.allocate(componentType.Size, componentType.Alignment);

    if (componentType.CopyConstructor != nullptr) {
        componentType.CopyConstructor(storedData, data);
    }
    else {
        memcpy(storedData, data, componentType.Size);
    }

    const UpdateCommand cmd {.type          = UpdateCommand::CommandType::AddComponent,
                             .dataSize      = static_cast<uint32_t>(componentType.Size),
                             .entity        = entity,
                             .componentHash = componentNameHash,
                             .dataPtr       = storedData};
    commandsQueue_->enqueue(cmd);
}

void ZArchetypeComponentRegistry::RemoveComponent(EcsEntity entity, size_t componentNameHash)
{
    const UpdateCommand cmd {
        .type          = UpdateCommand::CommandType::RemoveComponent,
        .entity        = entity,
        .componentHash = componentNameHash,
    };
    commandsQueue_->enqueue(cmd);
}

void ZArchetypeComponentRegistry::FlushUpdateCommands() noexcept
{
    const uint8_t readIdx      = writeIndex.load();
    const uint8_t nextWriteIdx = (readIdx + 1) % 2;

    writeIndex.store(nextWriteIdx);
    dataCopyAllocators_[nextWriteIdx].clear();

    while (const size_t count = commandsQueue_->try_dequeue_bulk(commandBuffer_.data(), kMaxBulkSize)) {
        for (size_t i = 0; i < count; ++i) {
            auto& curCommand = commandBuffer_[i];
            if (curCommand.type == UpdateCommand::CommandType::CreateEntity) {
                CreateEntityInternal(curCommand.entity);
            }
            else if (curCommand.type == UpdateCommand::CommandType::DestroyEntity) {
                RemoveEntityInternal(curCommand.entity);
            }
            else if (curCommand.type == UpdateCommand::CommandType::AddComponent) {
                AddComponentInternal(curCommand.entity, curCommand.componentHash, curCommand.dataPtr);
            }
            else {
                RemoveComponentInternal(curCommand.entity, curCommand.componentHash);
            }
        }
    }
}

ZArchetypePool& ZArchetypeComponentRegistry::GetOrCreatePool(const ZFixedBuffer<size_t, 32>& componentHashes)
{
    size_t newHash = 0;
    for (const auto& hash : componentHashes) {
        newHash = Fnv1aHashProvider::CombineHash(newHash, hash);
    }

    if (newHash == 0) {
        return *emptyEntities_;
    }

    if (archetypePools_.contains(newHash)) {
        return *archetypePools_[newHash];
    }

    auto pool = ZUniquePointer<ZArchetypePool>::make(componentHashes);
    for (auto& [queryHash, query] : cachedQueries_) {
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

    return *archetypePools_[newHash];
}

void ZArchetypeComponentRegistry::MoveEntity(EcsEntity entity, ZArchetypePool* oldPool, ZArchetypePool* newPool,
                                             void* newComponentData)
{
    auto newAlloc = newPool->AllocateEntity(entity);

    // Direct access by index (guaranteed to exist since called from a valid context)
    auto& oldRecord = entityIndex_[entity.entityId];

    const auto& newMetaList = newPool->GetComponentsMetadata();

    for (const auto& newMeta : newMetaList) {
        const size_t compHash = newMeta.typeInfo->HashCode;
        void* dstData = newPool->GetComponentData(newAlloc.chunk, newAlloc.index, newMeta);

        const auto* oldMeta = oldPool->GetMetadata(compHash);

        if (oldMeta) {
            void* srcData = oldPool->GetComponentData(oldRecord.chunk, oldRecord.rowIndex, *oldMeta);

            if (newMeta.typeInfo->MoveConstructor) {
                newMeta.typeInfo->MoveConstructor(dstData, srcData);
            }
            else {
                memcpy(dstData, srcData, newMeta.size);
            }
        }
        else {
            if (newComponentData) {
                if (newMeta.typeInfo->MoveConstructor) {
                    newMeta.typeInfo->MoveConstructor(dstData, newComponentData);
                }
                else {
                    memcpy(dstData, newComponentData, newMeta.size);
                }
            }
        }
    }

    RemoveEntityFromPool(entity);

    // Update the entry in the vector
    // IMPORTANT: We update the same memory location in the vector, not look it up again.
    entityIndex_[entity.entityId] = {
        .archetype = newPool, .chunk = newAlloc.chunk, .rowIndex = newAlloc.index, .generationId = entity.generationId};
}

void ZArchetypeComponentRegistry::ForEachComponent(const ForeachCallbackFunction& callback,
                                                   const ZFixedBuffer<size_t, 32>& requiredComponents)
{
    ZFixedBuffer<size_t, 32> requiredComponentsCopy = requiredComponents;
    std::ranges::sort(requiredComponentsCopy);
    size_t queryHash = 0;
    for (auto& hash : requiredComponentsCopy) {
        queryHash = Fnv1aHashProvider::CombineHash(queryHash, hash);
    }

    if (!cachedQueries_.contains(queryHash)) {
        LookupQuery query {
            .requiredComponentsHash = queryHash,
            .requiredComponents     = requiredComponents,
        };
        for (auto& [hash, pool] : archetypePools_) {
            bool suitable = true;
            for (const auto& requiredComponent : requiredComponents) {
                if (!pool->HasComponent(requiredComponent)) {
                    suitable = false;
                    break;
                }
            }
            if (!suitable) {
                continue;
            }
            query.pools.push_back(pool.get());
        }
        cachedQueries_[queryHash] = std::move(query);
    }

    auto& query = cachedQueries_[queryHash];
    ZFixedBuffer<void*, 32> dataPointers;
    for (const auto& pool : query.pools) {
        ZArchetypePool::ArchetypeChunk* chunk = pool->headChunk_;
        for (size_t i = 0; i < pool->chunksCount && chunk; ++i) {
            if (chunk->count > 0) {
                dataPointers.clear();
                for (const auto& component : requiredComponents) {
                    void* data = pool->GetRawComponentArray(chunk, component);
                    dataPointers.push_back(data);
                }
                callback(pool->GetEntityIdArray(chunk), dataPointers.begin(), chunk->count);
            }
            chunk = chunk->next;
        }
    }
}

void* ZArchetypeComponentRegistry::GetComponent(EcsEntity entity, size_t componentType)
{
    auto* record = GetEntityRecord(entity);
    if (record == nullptr) {
        return nullptr;
    }
    return record->archetype->GetComponentData(record->chunk, record->rowIndex,
                                               *record->archetype->GetMetadata(componentType));
}

void ZArchetypeComponentRegistry::RemoveEntityInternal(EcsEntity entity)
{
    // 1. Checking validity
    EntityRecord* record = GetEntityRecord(entity);
    if (record == nullptr) {
        return;
    }

    // 2. Removing from the pool (Swap & Pop)
    RemoveEntityFromPool(entity);

    record->archetype    = nullptr;
    record->generationId = 0;

    entity.generationId++;
    freeIndices_->enqueue(entity);
}

void ZArchetypeComponentRegistry::RemoveEntityFromPool(EcsEntity entity)
{
    EntityRecord& record = entityIndex_[entity.entityId];

    const EcsEntity movedEntity = record.archetype->DestroyEntity(record.chunk, record.rowIndex);

    if (movedEntity.entityId != entity.entityId) {
        if (movedEntity.entityId < entityIndex_.size()) {
            EntityRecord& movedRecord = entityIndex_[movedEntity.entityId];
            movedRecord.chunk         = record.chunk;
            movedRecord.rowIndex      = record.rowIndex;
        }
    }
}

void ZArchetypeComponentRegistry::CreateEntityInternal(EcsEntity entity)
{
    auto alloc = emptyEntities_->AllocateEntity(entity);
    if (entity.entityId > entityIndex_.size()) {
        entityIndex_.resize((entity.entityId + 1) * 2);
    }
    if (entity.entityId >= entityIndex_.size()) {
        size_t newSize = (entity.entityId + 1) * 2;
        entityIndex_.resize(newSize);
    }
    EntityRecord& record = entityIndex_[entity.entityId];
    record.archetype     = emptyEntities_.get();
    record.chunk         = alloc.chunk;
    record.rowIndex      = alloc.index;
    record.generationId  = entity.generationId;
}

void ZArchetypeComponentRegistry::AddComponentInternal(EcsEntity entity, size_t componentNameHash, void* data)
{
    EntityRecord* oldRecordPtr = GetEntityRecord(entity);
    if (oldRecordPtr == nullptr) {
        return;
    }
    const EntityRecord& oldRecord = *oldRecordPtr;

    auto* oldPool = oldRecord.archetype;

    const size_t oldHash = oldPool->GetKey();
    size_t newHash       = 0;

    bool transitionFound  = false;
    auto transitionFromIt = additionEdgesTransitions_.find(oldHash);
    if (transitionFromIt != additionEdgesTransitions_.end()) {
        auto transitionToIt = transitionFromIt->second.find(componentNameHash);
        if (transitionToIt != transitionFromIt->second.end()) {
            newHash         = transitionToIt->second;
            transitionFound = true;
        }
    }
    if (!transitionFound) {
        const auto& hashes = oldPool->GetComponentHashes();
        ZFixedBuffer<size_t, 32> hashesBuffer;
        for (auto hash : hashes) {
            hashesBuffer.push_back(hash);
        }
        hashesBuffer.push_back(componentNameHash);
        std::ranges::sort(hashesBuffer);

        auto& newPoolRef = GetOrCreatePool(hashesBuffer);
        newHash          = newPoolRef.GetKey();

        additionEdgesTransitions_[oldHash][componentNameHash] = newHash;
    }

    auto& newPool = archetypePools_[newHash];
    MoveEntity(entity, oldPool, newPool.get(), data);
}

void ZArchetypeComponentRegistry::RemoveComponentInternal(EcsEntity entity, size_t componentNameHash)
{
    EntityRecord* oldRecordPtr = GetEntityRecord(entity);
    if (oldRecordPtr == nullptr) {
        return;
    }
    const EntityRecord& oldRecord = *oldRecordPtr;

    ZArchetypePool* oldPool = oldRecord.archetype;

    const size_t oldHash = oldPool->GetKey();
    size_t newHash = 0;

    auto transitionFromIt = deletionEdgesTransitions_.find(oldHash);
    bool transitionFound  = false;
    if (transitionFromIt != deletionEdgesTransitions_.end()) {
        auto transitionToIt = transitionFromIt->second.find(componentNameHash);
        if (transitionToIt != transitionFromIt->second.end()) {
            newHash         = transitionToIt->second;
            transitionFound = true;
        }
    }

    if (!transitionFound) {
        const auto& hashes = oldPool->GetComponentHashes();
        ZFixedBuffer<size_t, 32> hashesBuffer;
        for (auto hash : hashes) {
            if (hash != componentNameHash) {
                hashesBuffer.push_back(hash);
            }
        }
        std::ranges::sort(hashesBuffer);
        auto& newPoolRef                                      = GetOrCreatePool(hashesBuffer);
        newHash                                               = newPoolRef.GetKey();
        deletionEdgesTransitions_[oldHash][componentNameHash] = newHash;
    }

    auto& newPool = archetypePools_[newHash];
    MoveEntity(entity, oldPool, newPool.get(), nullptr);
}