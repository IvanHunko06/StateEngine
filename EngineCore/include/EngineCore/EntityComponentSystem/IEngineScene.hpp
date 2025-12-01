#pragma once
#include "EcsEntity.hpp"
#include "IEcsSystem.hpp"
#include "EngineCore/EngineUpdatePhase.hpp"
#include "IPhaseUpdatePolicy.hpp"
namespace StateEngine::EngineCore::EntityComponentSystem {
	using EntityViewCallback = void(*)(EcsEntity entity, void** components, void* userData);
	class IEngineScene {
	public:
        virtual ~IEngineScene() = default;

        // --- Entity Management ---
        virtual EcsEntity createEntity() = 0;
        virtual void destroyEntity(EcsEntity entity) = 0;

        // --- Component Management ---
        virtual void addComponent(EcsEntity entity, size_t componentType, const void* componentData) = 0;
        virtual void removeComponent(EcsEntity entity, size_t componentType) = 0;
        virtual void* getComponent(EcsEntity entity, size_t componentType) = 0;
        virtual void forEachEntity(const size_t* componentTypes, size_t componentsCount, EntityViewCallback callback, void* userData) = 0;


        // --- System Management ---
        virtual void addSystem(EngineUpdatePhase phase, IEcsSystem* system) = 0;
        virtual void removeSystem(EngineUpdatePhase phase, IEcsSystem* system) = 0;


        // --- Policy ---
        virtual void setPhaseUpdatePolicy(IPhaseUpdatePolicy* policy) = 0;
	};
}