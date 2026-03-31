#pragma once
#include "AttributeBase.h"
#include "SerializableAttribute.h"
#include "NoSerializableAttribute.h"
#include "ClassSerializableAttribute.h"
#include "ReflectableAttribute.h"
#include "NoReflectableAttribute.h"
#include "ClassReflectableAttribute.h"
#include "ClassFinalReflectableAttribute.h"
#include "ClassFinalSerializableAttribute.h"
#include <cctype>

static AttributeHandler *GetAttributeHandler(AttributeKind kind)
{
    static SerializableHandler           serializable;
    static NoSerializableHandler         noSerializable;
    static ReflectableHandler            reflectable;
    static NoReflectableHandler          noReflectable;
    static ClassSerializableHandler      classSerializable;
    static ClassReflectableHandler       classReflectable;
    static ClassFinalReflectableHandler  classFinalReflectable;
    static ClassFinalSerializableHandler classFinalSerializable;
    switch (kind)
    {
        case SerializableHandler::kKind:           return &serializable;
        case NoSerializableHandler::kKind:         return &noSerializable;
        case ReflectableHandler::kKind:            return &reflectable;
        case NoReflectableHandler::kKind:          return &noReflectable;
        case ClassSerializableHandler::kKind:      return &classSerializable;
        case ClassReflectableHandler::kKind:       return &classReflectable;
        case ClassFinalReflectableHandler::kKind:  return &classFinalReflectable;
        case ClassFinalSerializableHandler::kKind: return &classFinalSerializable;
        default:                                   return nullptr;
    }
}

static const std::vector<AttributeKind> &GetAllKinds()
{
    static const std::vector<AttributeKind> kinds = {
        SerializableHandler::kKind,
        NoSerializableHandler::kKind,
        ReflectableHandler::kKind,
        NoReflectableHandler::kKind,
        ClassSerializableHandler::kKind,
        ClassReflectableHandler::kKind,
        ClassFinalReflectableHandler::kKind,
        ClassFinalSerializableHandler::kKind,
    };
    return kinds;
}

static const char *AttributeInternalName(AttributeKind kind)
{
    switch (kind)
    {
        case SerializableHandler::kKind:           return SerializableHandler::kName;
        case NoSerializableHandler::kKind:         return NoSerializableHandler::kName;
        case ReflectableHandler::kKind:            return ReflectableHandler::kName;
        case NoReflectableHandler::kKind:          return NoReflectableHandler::kName;
        case ClassSerializableHandler::kKind:      return ClassSerializableHandler::kName;
        case ClassReflectableHandler::kKind:       return ClassReflectableHandler::kName;
        case ClassFinalReflectableHandler::kKind:  return ClassFinalReflectableHandler::kName;
        case ClassFinalSerializableHandler::kKind: return ClassFinalSerializableHandler::kName;
        default:                                   return "";
    }
}

static std::string AttributeMacroName(AttributeKind kind)
{
    switch (kind)
    {
        case SerializableHandler::kKind:           return SerializableHandler::kMacroName;
        case NoSerializableHandler::kKind:         return NoSerializableHandler::kMacroName;
        case ReflectableHandler::kKind:            return ReflectableHandler::kMacroName;
        case NoReflectableHandler::kKind:          return NoReflectableHandler::kMacroName;
        case ClassSerializableHandler::kKind:      return ClassSerializableHandler::kMacroName;
        case ClassReflectableHandler::kKind:       return ClassReflectableHandler::kMacroName;
        case ClassFinalReflectableHandler::kKind:  return ClassFinalReflectableHandler::kMacroName;
        case ClassFinalSerializableHandler::kKind: return ClassFinalSerializableHandler::kMacroName;
        default:                                   return {};
    }
}

static AttributeKind GetFinalKindFor(AttributeKind kind)
{
    switch (kind)
    {
        case AttributeKind::ClassReflectable:  return AttributeKind::ClassFinalReflectable;
        case AttributeKind::ClassSerializable: return AttributeKind::ClassFinalSerializable;
        default:                               return AttributeKind::Unknown;
    }
}

// Conflict group ID: 0=none, 1=reflectable, 2=serializable
static int GetConflictGroupId(AttributeKind kind)
{
    switch (kind)
    {
        case AttributeKind::ClassReflectable:
        case AttributeKind::ClassFinalReflectable:
            return 1;
        case AttributeKind::ClassSerializable:
        case AttributeKind::ClassFinalSerializable:
            return 2;
        default:
            return 0;
    }
}

static const char *GetConflictGroupName(int gid)
{
    switch (gid)
    {
        case 1:  return "reflectable";
        case 2:  return "serializable";
        default: return "unknown";
    }
}

static bool IsFinalKind(AttributeKind kind)
{
    return kind == AttributeKind::ClassFinalReflectable ||
           kind == AttributeKind::ClassFinalSerializable;
}

static std::vector<AttributeInstance> ParseAttributeTokens(const std::string &content)
{
    std::vector<AttributeInstance> result;
    std::string token;
    int depth = 0;
    for (size_t i = 0; i <= content.size(); ++i)
    {
        char ch = (i < content.size()) ? content[i] : ',';
        if (ch == '(')
        {
            ++depth;
            token += ch;
        }
        else if (ch == ')')
        {
            if (depth > 0) --depth;
            token += ch;
        }
        else if (ch == ',' && depth == 0)
        {
            size_t a = 0, b = token.size();
            while (a < b && std::isspace((unsigned char)token[a])) ++a;
            while (b > a && std::isspace((unsigned char)token[b - 1])) --b;
            token = token.substr(a, b - a);
            if (!token.empty())
            {
                AttributeInstance attr;
                const auto paren = token.find('(');
                if (paren != std::string::npos)
                {
                    attr.name = token.substr(0, paren);
                    size_t nb = attr.name.size();
                    while (nb > 0 && std::isspace((unsigned char)attr.name[nb - 1])) --nb;
                    attr.name = attr.name.substr(0, nb);

                    const size_t close = token.rfind(')');
                    if (close != std::string::npos && close > paren)
                    {
                        std::string ps = token.substr(paren + 1, close - paren - 1);
                        std::string p;
                        for (char pc : ps + ",")
                        {
                            if (pc == ',')
                            {
                                size_t pa = 0, pb = p.size();
                                while (pa < pb && std::isspace((unsigned char)p[pa])) ++pa;
                                while (pb > pa && std::isspace((unsigned char)p[pb - 1])) --pb;
                                p = p.substr(pa, pb - pa);
                                if (!p.empty()) attr.params.push_back(p);
                                p.clear();
                            }
                            else
                            {
                                p += pc;
                            }
                        }
                    }
                }
                else
                {
                    attr.name = token;
                }
                attr.kind = AttributeKindFromName(attr.name);
                result.push_back(std::move(attr));
            }
            token.clear();
        }
        else
        {
            token += ch;
        }
    }
    return result;
}
