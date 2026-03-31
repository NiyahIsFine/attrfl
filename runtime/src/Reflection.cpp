#include "Reflection.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
using namespace attrfl;

static std::unordered_map<std::string, TypeInfo *> &NameMap()
{
    static std::unordered_map<std::string, TypeInfo *> m;
    return m;
}

static std::unordered_map<std::type_index, TypeInfo *> &IndexMap()
{
    static std::unordered_map<std::type_index, TypeInfo *> m;
    return m;
}

static std::vector<TypeInfo *> &AllTypes()
{
    static std::vector<TypeInfo *> v;
    return v;
}

bool Reflect::RegisterTypeInternal(TypeInfo *type, const std::string &typeName)
{
    if (!type) return false;
    if (NameMap().count(typeName) > 0)
    {
        std::cout << "[warn]" << typeName << " already registered" << std::endl;
        return false;
    }

    NameMap()[typeName] = type;
    IndexMap()[type->GetTypeIndex()] = type;
    AllTypes().push_back(type);
    return true;
}

TypeInfo *Reflect::GetType(const std::string &name)
{
    const auto it = NameMap().find(name);
    return it != NameMap().end() ? it->second : nullptr;
}

TypeInfo *Reflect::GetType(std::type_index index)
{
    const auto it = IndexMap().find(index);
    return it != IndexMap().end() ? it->second : nullptr;
}

const std::vector<TypeInfo *> &Reflect::GetAllTypesInternal()
{
    return AllTypes();
}

std::vector<std::string> Reflect::GetAllTypeNames()
{
    std::vector<std::string> names;
    names.reserve(NameMap().size());
    for (const auto &[name, _]: NameMap()) names.push_back(name);
    return names;
}


void Reflect::PrintAllTypes()
{
    std::cout << "=== Registered Types ({" << AllTypes().size() << "}) ===" << std::endl;
    for (TypeInfo *type: AllTypes())
    {
        std::cout << "  - {" << type->GetName() << "} (size: {" << type->GetSize() << "})" << std::endl;
    }
}
