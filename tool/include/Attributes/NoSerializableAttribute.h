#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"
#include <algorithm>

// Field-level: excludes field from serialization (only meaningful if already reflected)
struct NoSerializableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::NoSerializable;
    static constexpr const char *kName = "noSerializable";
    static constexpr const char *kMacroName = "EPNoSerializable";

    void OnField(FieldInfo &field, const AttributeInstance &attr) override
    {
        field.attributes.push_back(attr);
    }

    // Removes EPSerializable; if removed, adds EPReflectable to keep reflection visibility
    void OnPostField(FieldInfo &field, const AttributeInstance &) override
    {
        auto it = std::find_if(field.attributes.begin(), field.attributes.end(),
            [](const AttributeInstance &a) { return a.kind == AttributeKind::Serializable; });

        if (it == field.attributes.end()) return;

        field.attributes.erase(it);

        if (!field.HasAttribute(AttributeKind::Reflectable))
        {
            AttributeInstance refl;
            refl.kind = AttributeKind::Reflectable;
            refl.name = "reflectable";
            field.attributes.push_back(refl);
        }
    }

    bool IsPositiveForReflect() const override { return false; }
};
