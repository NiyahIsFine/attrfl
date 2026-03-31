#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Field-level: marks field as serializable (also collected by reflection)
struct SerializableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::Serializable;
    static constexpr const char *kName = "serializable";
    static constexpr const char *kMacroName = "EPSerializable";

    void OnField(FieldInfo &field, const AttributeInstance &attr) override
    {
        field.attributes.push_back(attr);
    }

};
