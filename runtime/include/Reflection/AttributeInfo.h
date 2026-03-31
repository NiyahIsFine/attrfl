#pragma once
#include <string>
#include <vector>

namespace attrfl
{
    enum class AttributeParamType : unsigned char
    {
        String = 0,
        Bool = 1,
        Int = 2,
        Float = 3,
    };

    struct AttributeParam
    {
        unsigned char _type = static_cast<unsigned char>(AttributeParamType::String);

        union Value
        {
            bool boolVal;
            int intVal;
            float floatVal;
        } value{};

        std::string strVal;


        static AttributeParam MakeString(std::string val)
        {
            AttributeParam p;
            p._type = static_cast<unsigned char>(AttributeParamType::String);
            p.strVal = std::move(val);
            return p;
        }

        static AttributeParam MakeBool(bool val)
        {
            AttributeParam p;
            p._type = static_cast<unsigned char>(AttributeParamType::Bool);
            p.value.boolVal = val;
            return p;
        }

        static AttributeParam MakeInt(int val)
        {
            AttributeParam p;
            p._type = static_cast<unsigned char>(AttributeParamType::Int);
            p.value.intVal = val;
            return p;
        }

        static AttributeParam MakeFloat(float val)
        {
            AttributeParam p;
            p._type = static_cast<unsigned char>(AttributeParamType::Float);
            p.value.floatVal = val;
            return p;
        }


        bool IsString() const { return _type == static_cast<unsigned char>(AttributeParamType::String); }
        bool IsBool() const { return _type == static_cast<unsigned char>(AttributeParamType::Bool); }
        bool IsInt() const { return _type == static_cast<unsigned char>(AttributeParamType::Int); }
        bool IsFloat() const { return _type == static_cast<unsigned char>(AttributeParamType::Float); }


        const std::string &AsString() const { return strVal; }
        bool AsBool() const { return value.boolVal; }
        int AsInt() const { return value.intVal; }
        float AsFloat() const { return value.floatVal; }
    };

    struct AttributeInfo
    {
        std::string name;
        std::vector<AttributeParam> params;

        bool HasParams() const { return !params.empty(); }

        const AttributeParam &Param(size_t i = 0) const
        {
            static const AttributeParam empty{};
            return i < params.size() ? params[i] : empty;
        }
    };
}
