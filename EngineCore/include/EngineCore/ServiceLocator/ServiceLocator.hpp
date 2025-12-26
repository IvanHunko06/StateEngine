#pragma once
#include "EngineCore/EngineTypeSystem/GetCompileTimeNames.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "ServiceLocatorExports.hpp"
#include <type_traits>

namespace StateEngine::EngineCore::ServiceLocator {
    template <typename T>
    concept ServiceLocatorRestrictionConcept =
        requires { EngineTypeSystem::TypeRegistryRestrictionConcept<T> && std::is_class_v<T>; };

    class ServiceLocator {
      public:
        template <typename T>
            requires ServiceLocatorRestrictionConcept<T>
        static T& GetRequiredService()
        {
            const EngineTypeSystem::TypeInfo& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            T* service           = reinterpret_cast<T*>(ServiceLocator_GetService(&type));
            if (service == nullptr) {
                assert(false && "Required service is not registered! The program will terminate!");
                std::abort();
            }
            return *service;
        }

        template <typename T>
            requires ServiceLocatorRestrictionConcept<T>
        static T* TryGetService()
        {
            const EngineTypeSystem::TypeInfo* type = EngineTypeSystem::TypeRegistry::TryGetType<T>();
            if (type == nullptr) {
                return nullptr;
            }
            T* service = reinterpret_cast<T*>(ServiceLocator_GetService(type));
            return service;
        }

        template <typename T>
            requires ServiceLocatorRestrictionConcept<T>
        static void RegisterService(T* instance)
        {
            const EngineTypeSystem::TypeInfo& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();

            ServiceLocator_RegisterService(&type, instance);
        }

        template <typename T>
            requires ServiceLocatorRestrictionConcept<T>
        static void RemoveService()
        {
            const EngineTypeSystem::TypeInfo* type = EngineTypeSystem::TypeRegistry::TryGetType<T>();
            if (type == nullptr) {
                return;
            }
            ServiceLocator_RemoveService(type);
        }
    };
}  // namespace StateEngine::EngineCore::ServiceLocator