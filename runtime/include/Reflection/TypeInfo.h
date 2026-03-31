#pragma once

#include <typeindex>
#include <functional>
#include "Reflection/FieldInfo.h"
#include "Reflection/ContainerTraits.h"
#include <string>
#include <vector>
#include <unordered_set>

namespace attrfl
{
    class TypeInfo
    {
    private:
        std::string name;
        std::type_index index;
        size_t size;
        std::vector<TypeInfo *> parents;

        void *(*constructor)();

        std::vector<FieldInfo> fields;
        mutable std::vector<FieldInfo> allFieldsCache;
        mutable bool allFieldsInit = false;
        std::vector<AttributeInfo> attributes;
        std::unordered_set<std::string> attributeSet;
        mutable std::unordered_set<const TypeInfo *> ancestorsCache;
        mutable bool ancestorsInit = false;

    public:
        ContainerKind containerKind = ContainerKind::None;
        TypeInfo *keyType = nullptr;
        TypeInfo *valueType = nullptr;

        std::function<size_t(const void *)> listGetSize;
        std::function<const void *(const void *, size_t)> listGetElement;
        std::function<void(void *)> listClear;
        std::function<void *(void *)> listAddDefault;

        std::function<void(const void *, std::function<void(const std::string &, const void *)>)> dictForEach;
        std::function<void(void *)> dictClear;
        std::function<void *(void *, std::string_view)> dictInsert;

        bool IsList() const { return containerKind == ContainerKind::List; }
        bool IsDict() const { return containerKind == ContainerKind::Dict; }

        TypeInfo(const std::string &typeName,
                 std::type_index typeIndex,
                 size_t typeSize,
                 void *(*ctor)())
            : name(typeName)
              , index(typeIndex)
              , size(typeSize)
              , constructor(ctor)
        {
        }

        ~TypeInfo() = default;

        const std::string &GetName() const { return name; }
        std::type_index GetTypeIndex() const { return index; }
        size_t GetSize() const { return size; }

        TypeInfo *GetParent() const { return parents.empty() ? nullptr : parents.front(); }
        const std::vector<TypeInfo *> &GetParents() const { return parents; }

        void AddParent(TypeInfo *parentType)
        {
            parents.emplace_back(parentType);
        }

        bool CanCreate() const { return constructor != nullptr; }

        void *CreateInstance() const
        {
            if (!constructor)
                return nullptr;
            return constructor();
        }


        void AddField(FieldInfo field)
        {
            fields.emplace_back(std::move(field));
        }

        void AddAttribute(AttributeInfo attr)
        {
            attributeSet.insert(attr.name);
            attributes.emplace_back(std::move(attr));
        }

        const std::vector<AttributeInfo> &GetAttributes() const { return attributes; }

        bool HasAttribute(const std::string &macroName) const
        {
            return attributeSet.count(macroName) > 0;
        }

        const AttributeInfo *FindAttribute(const std::string &macroName) const
        {
            for (const auto &a: attributes)
                if (a.name == macroName) return &a;
            return nullptr;
        }

        const std::vector<FieldInfo> &GetFields() const { return fields; }

        const std::vector<FieldInfo> &GetAllFields() const
        {
            if (!allFieldsInit)
            {
                for (const auto *parent: parents)
                {
                    const auto &parentsFields = parent->GetAllFields();
                    allFieldsCache.insert(allFieldsCache.end(), parentsFields.begin(), parentsFields.end());
                }
                allFieldsCache.insert(allFieldsCache.end(), fields.begin(), fields.end());
                allFieldsInit = true;
            }
            return allFieldsCache;
        }

        const FieldInfo *FindField(const std::string &fieldName) const
        {
            for (const auto &f: GetAllFields())
                if (f.name == fieldName) return &f;

            return nullptr;
        }

        const std::unordered_set<const TypeInfo *> &GetAncestors() const
        {
            if (!ancestorsInit)
            {
                ancestorsCache.insert(this);
                for (const auto *parent: parents)
                {
                    if (!parent) continue;
                    const auto &parentsAncestors = parent->GetAncestors();
                    ancestorsCache.insert(parentsAncestors.begin(), parentsAncestors.end());
                }
                ancestorsInit = true;
            }
            return ancestorsCache;
        }

        bool IsA(const TypeInfo *other) const
        {
            if (!other) return false;
            return GetAncestors().count(other) > 0;
        }

        bool IsDerivedFrom(const TypeInfo *base) const
        {
            if (!base) return false;
            // 自身不算派生自自身
            if (this == base || index == base->index) return false;
            return GetAncestors().count(base) > 0;
        }

        bool IsExactly(const TypeInfo *other) const
        {
            if (!other) return false;
            return this == other || index == other->index;
        }

        bool operator==(const TypeInfo &other) const
        {
            return index == other.index;
        }

        bool operator!=(const TypeInfo &other) const
        {
            return index != other.index;
        }
    };
}
