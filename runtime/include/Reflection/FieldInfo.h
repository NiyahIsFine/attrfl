#pragma once

#include <vector>
#include <unordered_set>
#include "Reflection/AttributeInfo.h"
#include "Reflection/ContainerTraits.h"
#include <string>

namespace attrfl
{
    class TypeInfo;

    struct FieldInfo
    {
        enum class Access
        {
            Public,
            Protected,
            Private
        };

        std::string name;
        TypeInfo *type;
        Access access;

        size_t offset = 0;
        size_t fieldSize = 0;

        std::vector<AttributeInfo> attributes;

        std::unordered_set<std::string> attributeSet;

        ContainerKind containerKind = ContainerKind::None;

        bool IsList() const { return containerKind == ContainerKind::List; }
        bool IsDict() const { return containerKind == ContainerKind::Dict; }
        bool IsContainer() const { return containerKind != ContainerKind::None; }

        void RebuildAttributeSet()
        {
            attributeSet.clear();
            for (const auto &a: attributes)
                attributeSet.insert(a.name);
        }

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

        void *GetRawPtr(void *obj) const
        {
            return static_cast<char *>(obj) + offset;
        }

        const void *GetRawPtr(const void *obj) const
        {
            return static_cast<const char *>(obj) + offset;
        }

        template<typename F>
        F &GetValue(void *obj) const
        {
            return *static_cast<F *>(GetRawPtr(obj));
        }

        template<typename F>
        const F &GetValue(const void *obj) const
        {
            return *static_cast<const F *>(GetRawPtr(obj));
        }

        template<typename F>
        void SetValue(void *obj, const F &val) const
        {
            *static_cast<F *>(GetRawPtr(obj)) = val;
        }
    };
}
