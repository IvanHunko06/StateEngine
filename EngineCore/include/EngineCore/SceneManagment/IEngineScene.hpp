#pragma once
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"
#include "EngineCore/EntityComponentSystem/IEcsSystem.hpp"
#include "EngineCore/EngineUpdatePhase.hpp"
#include "PhaseUpdatePolicy.hpp"
#include "EngineCore/EntityComponentSystem/ForeachCallbackFunction.hpp"

using StateEngine::EngineCore::EntityComponentSystem::EcsEntity;
using StateEngine::EngineCore::EntityComponentSystem::ForeachCallbackFunction;
using StateEngine::EngineCore::EntityComponentSystem::IEcsSystem;
namespace StateEngine::EngineCore::SceneManagment {
	class IEngineScene {
	public:
        virtual ~IEngineScene() = default;
        virtual const char* GetSceneName() = 0;

        // --- Entity Management ---
        virtual EcsEntity CreateEntity() = 0;
        virtual void DestroyEntity(EcsEntity entity) = 0;

        // --- Component Management ---
        virtual void AddComponent(EcsEntity entity, size_t componentType, const void* componentData) = 0;
        virtual void RemoveComponent(EcsEntity entity, size_t componentType) = 0;
        virtual void* GetComponent(EcsEntity entity, size_t componentType) = 0;
        virtual void ForEachComponent(const ForeachCallbackFunction& callback, const size_t* components, size_t componentsCount) = 0;


        // --- System Management ---
        virtual void AddSystem(EngineUpdatePhase phase, IEcsSystem* system) = 0;
        virtual void RemoveSystem(EngineUpdatePhase phase, IEcsSystem* system) = 0;


        // --- Policy ---
        virtual void SetPhaseUpdatePolicy(const PhaseUpdatePolicy& policy) = 0;
	};
}