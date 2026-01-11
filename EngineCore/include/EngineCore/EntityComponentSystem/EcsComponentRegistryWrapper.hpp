#pragma once
#include "EngineCore/EngineTypeSystem/GetCompileTimeNames.hpp"
#include "EngineCore/EntityComponentSystem/EcsEntity.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "IEcsComponentRegistry.hpp"
#include <concepts>
#include <type_traits>
#include <utility>

namespace StateEngine::EngineCore::EntityComponentSystem {
    template <typename T>
    concept IsEcsComponent = std::is_class_v<T>;

    namespace {
        template <typename...>
        struct IsUnique : std::true_type {};

        template <typename T, typename... Rest>
        struct IsUnique<T, Rest...> :
            std::bool_constant<(!std::is_same_v<T, Rest> && ...) && IsUnique<Rest...>::value> {};
    }  // namespace

    template <typename... Ts>
    concept AreUnique = IsUnique<Ts...>::value;

    template <typename Func, typename... Components>
    concept SystemCallback = std::invocable<Func, const EcsEntity*, Components*..., size_t>;

    struct EcsComponentRegistryWrapper {
        static EcsEntity CreateEntity(IEcsComponentRegistry* registry)
        {
            return registry->CreateEntity();
        }
        static void DestroyEntity(IEcsComponentRegistry* registry, EcsEntity entity)
        {
            registry->DestroyEntity(entity);
        }

        template <typename T>
            requires IsEcsComponent<T>
        static void AddComponent(IEcsComponentRegistry* registry, EcsEntity entity, const T& componentData)
        {
            size_t componentTypeHash = Hashing::Fnv1aHashProvider::HashString(EngineTypeSystem::GetTypeName<T>());
            registry->AddComponent(entity, componentTypeHash, &componentData);
        }
        template <typename T>
            requires IsEcsComponent<T>
        static void RemoveComponent(IEcsComponentRegistry* registry, EcsEntity entity)
        {
            size_t componentTypeHash = Hashing::Fnv1aHashProvider::HashString(EngineTypeSystem::GetTypeName<T>());
            registry->RemoveComponent(entity, componentTypeHash);
        }

        template <typename... TComponents, typename TFunc>
            requires(IsEcsComponent<TComponents> && ...) &&  // Все T - структуры
                    AreUnique<TComponents...> &&             // Нет дубликатов
                    SystemCallback<TFunc, TComponents...>    // Лямбда имеет верную сигнатуру
        static void ForEachComponent(IEcsComponentRegistry* registry, TFunc&& callback)
        {
            IEcsComponentRegistry::ForEeachComponentsBuffer componentsBuffer;
            (componentsBuffer.push_back(
                 Hashing::Fnv1aHashProvider::HashString(EngineTypeSystem::GetTypeName<TComponents>())),
             ...);

            auto typedAdapter = [&](const EcsEntity* entities, void** rawComponents, size_t count) {
                InvokeWithCastedArgs<TComponents...>(std::forward<TFunc>(callback), entities, rawComponents, count,
                                                     std::make_index_sequence<sizeof...(TComponents)> {});
            };

            registry->ForEachComponent(typedAdapter, componentsBuffer);
        }
        template <typename T>
            requires IsEcsComponent<T>
        static T* GetComponent(IEcsComponentRegistry* registry, EcsEntity entity)
        {
            size_t componentTypeHash = Hashing::Fnv1aHashProvider::HashString(EngineTypeSystem::GetTypeName<T>());
            return static_cast<T*>(registry->GetComponent(entity, componentTypeHash));
        }

      private:
        template <typename... TComponents, typename TFunc, size_t... Is>
        static void InvokeWithCastedArgs(TFunc&& callback, const EcsEntity* entities, void** rawComponents, size_t count,
                                         std::index_sequence<Is...>)
        {
            // Вот здесь происходит магия приведения типов:
            // rawComponents[0] -> TComponents_0*
            // rawComponents[1] -> TComponents_1*
            callback(entities, static_cast<TComponents*>(rawComponents[Is])..., count);
        }
    };
}  // namespace StateEngine::EngineCore::EntityComponentSystem