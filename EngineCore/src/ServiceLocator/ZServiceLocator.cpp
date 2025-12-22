#include "ZServiceLocator.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include <cassert>

using namespace StateEngine::EngineCore::ServiceLocator;

ZHashMap<const TypeInfo*, void*> ZServiceLocator::RegisteredServices;
bool ZServiceLocator::IsSealed;

void ZServiceLocator::RegisterService(const TypeInfo* type, void* instance)
{
    assert(type && "type is null");
    assert(instance && "instance is null");
    if (type == nullptr || instance == nullptr) {
        return;
    }
    if (IsSealed) {
        assert(!IsSealed && "Cannot register service after initialization!");
        return;
    }
    if (RegisteredServices.contains(type)) {
        ZLOG_ERROR("EngineCore") << "Service '" << type->Name.c_str() << "' is being registered twice!";
        assert(false && "Duplicate service registration!");
        return;
    }
    RegisteredServices[type] = instance;
}
void* ZServiceLocator::GetService(const TypeInfo* type)
{
    assert(type && "type is null");
    if (type == nullptr) {
        return nullptr;
    }
    auto it = RegisteredServices.find(type);
    if (it == RegisteredServices.end()) {
        return nullptr;
    }
    return it->second;
}
void ZServiceLocator::RemoveService(const TypeInfo* type)
{
    assert(type && "type is null");
    if (type == nullptr) {
        return;
    }
    if (IsSealed) {
        assert(!IsSealed && "Cannot remove service after initialization!");
        return;
    }
    RegisteredServices.erase(type);
}