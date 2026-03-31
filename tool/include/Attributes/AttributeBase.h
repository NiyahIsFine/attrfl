#pragma once
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include <string>
#include <vector>
using namespace rapidjson;

struct FieldInfo;
struct ClassInfo;

enum class AttributeKind
{
    Unknown = 0,
    // Field-level
    Serializable,
    NoSerializable,
    Reflectable,
    NoReflectable,
    // Class-level
    ClassSerializable,
    ClassReflectable,
    // Class-level final (blocks propagation to subclasses)
    ClassFinalReflectable,
    ClassFinalSerializable,
};

static AttributeKind AttributeKindFromName(const std::string &name)
{
    if (name == "serializable") return AttributeKind::Serializable;
    if (name == "noSerializable") return AttributeKind::NoSerializable;
    if (name == "reflectable") return AttributeKind::Reflectable;
    if (name == "noReflectable") return AttributeKind::NoReflectable;
    if (name == "classSerializable") return AttributeKind::ClassSerializable;
    if (name == "classReflectable") return AttributeKind::ClassReflectable;
    if (name == "classFinalReflectable") return AttributeKind::ClassFinalReflectable;
    if (name == "classFinalSerializable") return AttributeKind::ClassFinalSerializable;
    return AttributeKind::Unknown;
}


struct AttributeInstance
{
    AttributeKind kind = AttributeKind::Unknown;
    std::string name;
    std::vector<std::string> params;

    bool HasParam() const { return !params.empty(); }

    const std::string &Param(size_t i = 0) const
    {
        static const std::string empty;
        return i < params.size() ? params[i] : empty;
    }

    std::string MacroName() const
    {
        if (name.empty()) return {};
        std::string m = "EP";
        m += (char) std::toupper((unsigned char) name[0]);
        m += name.substr(1);
        return m;
    }

    template<typename Writer>
    void Serialize(Writer &writer) const
    {
        writer.StartObject();
        writer.String("name");
        writer.String(name);
        writer.String("params");
        writer.StartArray();
        for (const auto &p: params) writer.String(p);
        writer.EndArray();
        writer.EndObject();
    }

    void Deserialize(const Value &v)
    {
        name = v["name"].GetString();
        kind = AttributeKindFromName(name);
        params.clear();
        if (v.HasMember("params"))
            for (const auto &p: v["params"].GetArray())
                params.push_back(p.GetString());
    }
};


inline bool ParseBoolParam(const AttributeInstance &attr, size_t index)
{
    if (attr.params.size() <= index) return false;
    const std::string &v = attr.Param(index);
    return v == "true" || v == "1";
}


struct AttributeHandler
{
    virtual ~AttributeHandler() = default;

    // Called when this attribute appears on a field
    virtual void OnField(FieldInfo &field, const AttributeInstance &attr)
    {
    }

    // Called when this attribute appears on a class
    virtual void OnClass(ClassInfo &cls, const AttributeInstance &attr)
    {
    }

    // Called after all attributes on a field are parsed; resolves priority
    virtual void OnPostField(FieldInfo &field, const AttributeInstance &attr)
    {
    }

    // Whether this attribute propagates to subclasses
    virtual bool ShouldPropagate(const AttributeInstance &attr) const { return false; }

    // Whether this class-level attribute triggers force_collect_all
    virtual bool ContributesFieldCollect(const AttributeInstance &attr) const { return false; }

    // Whether this is a positive (enabling) attribute for reflection
    virtual bool IsPositiveForReflect() const { return true; }

    // Field-level kind implied on each field when force_collect_all is active
    // Returns Unknown if none
    virtual AttributeKind GetImpliedFieldKind() const { return AttributeKind::Unknown; }
};
