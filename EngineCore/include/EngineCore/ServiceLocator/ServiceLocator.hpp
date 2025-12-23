#pragma once
#include "EngineCore/EngineTypeSystem/GetCompileTimeNames.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "ServiceLocatorExports.hpp"

namespace StateEngine::EngineCore::ServiceLocator {
    class ServiceLocator {
      public:
        template <typename T>
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T> && std::is_class_v<T>)
        static T& GetRequiredService()
        {
            const TypeInfo& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            T* service           = reinterpret_cast<T*>(ServiceLocator_GetService(&type));
            if (service == nullptr) {
                assert(false && "Required service is not registered! The program will terminate!");
                std::abort();
            }
            return *service;
        }

        template <typename T>
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T> && std::is_class_v<T>)
        static T* TryGetService()
        {
            const TypeInfo* type = EngineTypeSystem::TypeRegistry::TryGetType<T>();
            if (type == nullptr) {
                return nullptr;
            }
            T* service = reinterpret_cast<T*>(ServiceLocator_GetService(type));
            return service;
        }

        template <typename T>
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T> && std::is_class_v<T>)
        static void RegisterService(T* instance)
        {
            const TypeInfo& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();

            ServiceLocator_RegisterService(&type, instance);
        }

        template <typename T>
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T> && std::is_class_v<T>)
        static void RemoveService()
        {
            const TypeInfo* type = EngineTypeSystem::TypeRegistry::TryGetType<T>();
            if (type == nullptr) {
                return;
            }
            ServiceLocator_RemoveService(type);
        }
    };
}  // namespace StateEngine::EngineCore::ServiceLocator