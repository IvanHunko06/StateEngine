#include "ZArchetypePool.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include <cstring>
#include <cstdint>


using namespace StateEngine::EngineCore::EntityComponentSystem;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using StateEngine::EngineCore::Hashing::Fnv1aHashProvider;

ZArchetypePool::ZArchetypePool(const ZFixedBuffer <size_t, 32>& components) {
    for (const auto& component : components) {
        componentNamesHashCodes_.insert(component);
    }
	if (components.size() > 0) {
		for (size_t i = 0; i < components.size(); ++i) {
            const TypeInfo& typeInfo                = TypeRegistry::GetRequiredType(components[i]);
			archetypeKey = Fnv1aHashProvider::CombineHash(archetypeKey, components[i]);
            componentHashToIndexMap_[components[i]] = i;
			ComponentMetadata metadata{
				.size = typeInfo.Size,
				.alignment = typeInfo.Alignment,
				.typeInfo = &typeInfo
			};
			components_.push_back(metadata);
		}
	}
    CalculateLayout();
}

ZArchetypePool::~ZArchetypePool() {
    ArchetypeChunk* currentChunk = headChunk_;
    while (currentChunk){
        ArchetypeChunk* nextChunk = currentChunk->next;
        MemoryAllocator_Deallocate(currentChunk);
        currentChunk = nextChunk;
    }
}
ZArchetypePool::AllocResult ZArchetypePool::AllocateEntity(EcsEntity entityID) {
    ArchetypeChunk* chunk = headChunk_;

    if (!chunk || chunk->count == chunkCapacity_) {
        ArchetypeChunk* newChunk = AllocateChunk();
        newChunk->next = headChunk_;
        headChunk_ = newChunk;
        chunk = headChunk_;
    }

    const uint32_t index = chunk->count;

    // Записываем EntityID в скрытый массив (это важно для Destroy!)
    // Вычисляем адрес: Base + Offset_IDs + (Index * sizeof(EcsEntity))
    auto* buffer = reinterpret_cast<uint8_t*>(chunk);
    auto* ids = reinterpret_cast<EcsEntity*>(buffer + entityIdsOffset_);
    ids[index] = entityID;

    chunk->count++;

    return {.chunk = chunk, .index = index};
}
EcsEntity ZArchetypePool::DestroyEntity(ArchetypeChunk* chunk, uint32_t index) {
    const uint32_t lastIndex = chunk->count - 1;
    auto* buffer = reinterpret_cast<uint8_t*>(chunk);
    auto* ids = reinterpret_cast<EcsEntity*>(buffer + entityIdsOffset_);

    EcsEntity movedEntityId = ids[lastIndex]; // ID сущности, которую мы будем двигать

    // Если удаляемая сущность НЕ последняя, делаем Swap-and-Pop
    if (index != lastIndex) {
        // 1. Перемещаем ID сущности
        ids[index] = ids[lastIndex];

        // 2. Перемещаем данные всех компонентов
        for (const auto& meta : components_) {
            uint8_t* componentArrayStart = buffer + meta.chunkOffset;

            uint8_t* dst = componentArrayStart + (index * meta.size);
            uint8_t* src = componentArrayStart + (lastIndex * meta.size);
            if (meta.typeInfo->MoveConstructor != nullptr) {
                meta.typeInfo->MoveConstructor(dst, src);
            }
            else {
                std::memcpy(dst, src, meta.size);
            }
                

            if (meta.typeInfo->Destructor != nullptr) {
                meta.typeInfo->Destructor(src);
                std::memset(src, 0, meta.size);
            }
        }

        ids[lastIndex] = EcsEntity {.generationId = 0, .entityId = 0};  // Очистка на всякий случай
    }
    chunk->count--;

    // Возвращаем ID сущности, которая переехала (или удаляемой, если она была последней),
    // чтобы Registry мог обновить индекс.
    return movedEntityId;
}
ZArchetypePool::ArchetypeChunk* ZArchetypePool::AllocateChunk() {
    void* mem = MemoryAllocator_Calloc(1, chunkSize_, 64);

    ArchetypeChunk* chunk = new (mem) ArchetypeChunk();
    chunk->count = 0;
    chunk->next = nullptr;
    chunk->index = chunksCount++;
    return chunk;
}
void ZArchetypePool::CalculateLayout() {
    chunkSize_ = 16 * 1024; // Стандартный размер 16KB

    // 1. Считаем размер "одной строки" данных (EntityID + все компоненты)
    size_t bytesPerEntity = sizeof(EcsEntity);
    for (const auto& meta : components_) {
        bytesPerEntity += meta.size;
    }

    // 2. Первичное приближение Capacity (сколько в теории влезает)
    const size_t availableSpace = chunkSize_ - sizeof(ArchetypeChunk);
    chunkCapacity_ = availableSpace / bytesPerEntity;

    // 3. Уточняем Capacity с учетом выравнивания (Alignment Padding waste)
    // Цикл while нужен, так как паддинг может съесть место, и capacity придется уменьшить
    while (chunkCapacity_ > 0) {
        size_t currentOffset = sizeof(ArchetypeChunk);

        // --- Размещаем массив EntityID ---
        // EntityID требует выравнивания alignof(EcsEntity)
        currentOffset = AlignUp(currentOffset, alignof(EcsEntity));
        entityIdsOffset_ = currentOffset;
        currentOffset += sizeof(EcsEntity) * chunkCapacity_;

        // --- Размещаем массивы Компонентов ---
        bool fits = true;
        for (auto& meta : components_) {
            currentOffset = AlignUp(currentOffset, meta.alignment);
            meta.chunkOffset = currentOffset; // Запоминаем оффсет НАЧАЛА массива
            currentOffset += meta.size * chunkCapacity_;

            if (currentOffset > chunkSize_) {
                fits = false;
                break;
            }
        }

        if (fits) {
            break; // Отлично, текущий capacity влезает
        }
        chunkCapacity_--; // Уменьшаем и пробуем пересчитать оффсеты
    }
}

