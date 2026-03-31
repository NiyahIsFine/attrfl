#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Class-level: param0=childApply (propagate to subclasses)
// Always force-collects all fields as serializable
struct ClassSerializableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::ClassSerializable;
    static constexpr const char *kName = "classSerializable";
    static constexpr const char *kMacroName = "EPClassSerializable";

    void OnClass(ClassInfo &cls, const AttributeInstance &attr) override
    {
        cls.attributes.push_back(attr);
    }

    // childApply = param0
    bool ShouldPropagate(const AttributeInstance &attr) const override
    {
        return ParseBoolParam(attr, 0);
    }

    // Always triggers force_collect_all
    bool ContributesFieldCollect(const AttributeInstance &) const override
    {
        return true;
    }

    // Implies EPSerializable on each field when force-collecting
    AttributeKind GetImpliedFieldKind() const override { return AttributeKind::Serializable; }
};
