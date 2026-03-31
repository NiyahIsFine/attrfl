#pragma once
#include "AttributeBase.h"
#include "../ClassInfo.h"

// Class-level final: stops EPClassReflectable propagation to subclasses
// param0: inheritSelf — true=this class still inherits ancestor's EPClassReflectable,
//   false=this class drops it entirely. Does not propagate or trigger field collection.
struct ClassFinalReflectableHandler final : AttributeHandler
{
    static constexpr AttributeKind kKind = AttributeKind::ClassFinalReflectable;
    static constexpr const char *kName = "classFinalReflectable";
    static constexpr const char *kMacroName = "EPClassFinalReflectable";

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
