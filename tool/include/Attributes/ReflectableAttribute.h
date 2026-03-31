#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Field-level: marks field as reflectable
struct ReflectableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::Reflectable;
    static constexpr const char *kName = "reflectable";
    static constexpr const char *kMacroName = "EPReflectable";

    void OnField(FieldInfo &field, const AttributeInstance &attr) override
    {
        field.attributes.push_back(attr);
    }

};
