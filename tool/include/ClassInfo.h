#pragma once
#include "Attributes/AttributeBase.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include <string>
#include <vector>
using namespace rapidjson;

struct FieldInfo
{
    std::string name;
    std::string type;
    std::string access; // "public" / "protected" / "private"
    std::vector<AttributeInstance> attributes;

    bool HasAttribute(AttributeKind k) const
    {
        for (const auto &a: attributes)
            if (a.kind == k) return true;
        return false;
    }

    const AttributeInstance *FindAttribute(AttributeKind k) const
    {
        for (const auto &a: attributes)
            if (a.kind == k) return &a;
        return nullptr;
    }

    template<typename Writer>
    void Serialize(Writer &writer) const
    {
        writer.StartObject();
        writer.String("name");
        writer.String(name);
        writer.String("type");
        writer.String(type);
        writer.String("access");
        writer.String(access);
        writer.String("attributes");
        writer.StartArray();
        for (const auto &a: attributes) a.Serialize(writer);
        writer.EndArray();
        writer.EndObject();
    }

    void Deserialize(const Value &v)
    {
        name = v["name"].GetString();
        type = v["type"].GetString();
        access = v["access"].GetString();
        attributes.clear();
        if (v.HasMember("attributes"))
            for (const auto &a: v["attributes"].GetArray())
            {
                AttributeInstance ai;
                ai.Deserialize(a);
                attributes.push_back(std::move(ai));
            }
    }
};


struct ClassInfo
{
    std::string qualifiedName;
    std::string name;
    std::string ns;
    std::vector<std::string> includes;
    std::vector<std::string> bases;
    bool reflectable = false;
    bool external = false;
    std::vector<AttributeInstance> attributes;
    std::vector<FieldInfo> fields;

    bool HasAttribute(AttributeKind k) const
    {
        for (const auto &a: attributes)
            if (a.kind == k) return true;
        return false;
    }

    const AttributeInstance *FindAttribute(AttributeKind k) const
    {
        for (const auto &a: attributes)
            if (a.kind == k) return &a;
        return nullptr;
    }

    template<typename Writer>
    void Serialize(Writer &writer) const
    {
        writer.StartObject();

        writer.String("qualified_name");
        writer.String(qualifiedName);

        writer.String("name");
        writer.String(name);

        writer.String("ns");
        writer.String(ns);

        writer.String("includes");
        writer.StartArray();
        for (const auto &inc: includes) writer.String(inc);
        writer.EndArray();

        writer.String("bases");
        writer.StartArray();
        for (const auto &base: bases) writer.String(base);
        writer.EndArray();

        writer.String("reflectable");
        writer.Bool(reflectable);

        writer.String("attributes");
        writer.StartArray();
        for (const auto &a: attributes) a.Serialize(writer);
        writer.EndArray();

        writer.String("fields");
        writer.StartArray();
        for (const auto &f: fields) f.Serialize(writer);
        writer.EndArray();

        writer.EndObject();
    }

    void Deserialize(const Value &value)
    {
        qualifiedName = value["qualified_name"].GetString();

        name = value["name"].GetString();

        ns = value["ns"].GetString();

        includes.clear();
        for (const auto &inc: value["includes"].GetArray())
            includes.push_back(inc.GetString());

        bases.clear();
        for (const auto &base: value["bases"].GetArray())
            bases.push_back(base.GetString());

        reflectable = value["reflectable"].GetBool();

        attributes.clear();
        if (value.HasMember("attributes"))
            for (const auto &a: value["attributes"].GetArray())
            {
                AttributeInstance ai;
                ai.Deserialize(a);
                attributes.push_back(std::move(ai));
            }

        fields.clear();
        if (value.HasMember("fields"))
            for (const auto &f: value["fields"].GetArray())
            {
                FieldInfo fi;
                fi.Deserialize(f);
                fields.push_back(std::move(fi));
            }
    }
};

struct TargetClassDetail
{
    std::string target;
    long long generatedAt;
    std::vector<ClassInfo> classes;

    template<typename Writer>
    void Serialize(Writer &writer) const
    {
        writer.StartObject();

        writer.String("target");
        writer.String(target);

        writer.String("generated_at");
        writer.Int64(generatedAt);


        writer.String("classes");
        writer.StartArray();
        for (const auto &cls: classes)
        {
            cls.Serialize(writer);
        }
        writer.EndArray();

        writer.EndObject();
    }

    void Deserialize(const Document &document)
    {
        target = document["target"].GetString();
        generatedAt = document["generated_at"].GetInt64();
        classes.clear();
        for (const auto &cls: document["classes"].GetArray())
        {
            ClassInfo info;
            info.Deserialize(cls);
            classes.push_back(std::move(info));
        }
    }
};
