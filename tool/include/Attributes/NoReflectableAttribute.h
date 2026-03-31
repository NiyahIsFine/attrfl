#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Field-level: highest priority — disables both reflection and serialization
struct NoReflectableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::NoReflectable;
    static constexpr const char *kName = "noReflectable";
    static constexpr const char *kMacroName = "EPNoReflectable";

    void OnField(FieldInfo &field, const AttributeInstance &attr) override
    {
        field.attributes.push_back(attr);
    }

    // Highest priority: clear all other attributes, keep only EPNoReflectable
    void OnPostField(FieldInfo &field, const AttributeInstance &attr) override
    {
        field.attributes.clear();
        field.attributes.push_back(attr);
    }


    bool IsPositiveForReflect() const override { return false; }
};
