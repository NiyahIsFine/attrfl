#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Class-level: param0=childApply, param1=fieldCollect
struct ClassReflectableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::ClassReflectable;
    static constexpr const char *kName = "classReflectable";
    static constexpr const char *kMacroName = "EPClassReflectable";

    void OnClass(ClassInfo &cls, const AttributeInstance &attr) override
    {
        cls.attributes.push_back(attr);
    }

    // childApply = param0
    bool ShouldPropagate(const AttributeInstance &attr) const override
    {
        return ParseBoolParam(attr, 0);
    }

    // fieldCollect = param1 (default false)
    bool ContributesFieldCollect(const AttributeInstance &attr) const override
    {
        return ParseBoolParam(attr, 1);
    }

    // Implies EPReflectable on each field when force-collecting
    AttributeKind GetImpliedFieldKind() const override { return AttributeKind::Reflectable; }
};
