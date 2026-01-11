#pragma once
#include <cstdint>
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
using StateEngine::EngineCore::Hashing::Fnv1aHashProvider;
namespace StateEngine::EngineCore::EntityComponentSystem {
	struct EcsEntity {
		uint32_t generationId{ 0 };
        uint32_t entityId {0};

		inline size_t Hash() const noexcept {
            return *reinterpret_cast<const size_t*>(this);
        }

		inline bool operator==(const EcsEntity& other) const noexcept{
			return entityId == other.entityId && generationId == other.generationId;
		}
		inline bool operator!=(const EcsEntity& other) const noexcept{
			return !(*this == other);
		}
	};
}