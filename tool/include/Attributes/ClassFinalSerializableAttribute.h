#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Class-level final: stops EPClassSerializable propagation to subclasses
// param0: inheritSelf — true=this class still inherits ancestor's EPClassSerializable,
//   false=this class drops it entirely. Does not propagate or trigger field collection.
struct ClassFinalSerializableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::ClassFinalSerializable;
    static constexpr const char *kName = "classFinalSerializable";
    static constexpr const char *kMacroName = "EPClassFinalSerializable";

    void OnClass(ClassInfo &cls, const AttributeInstance &attr) override
    {
        cls.attributes.push_back(attr);
    }

    // Does not propagate to subclasses
    bool ShouldPropagate(const AttributeInstance &) const override { return false; }

    // Does not trigger field collection
    bool ContributesFieldCollect(const AttributeInstance &) const override { return false; }

    // Positive attribute, kept in final output
    bool IsPositiveForReflect() const override { return true; }
};
