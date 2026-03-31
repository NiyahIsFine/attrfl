#pragma once
#include "ClassInfo.h"
#include "Attributes/AttributeRegistry.h"
#include "rapidjson/istreamwrapper.h"
#include <cctype>
#include <unordered_set>

using namespace rapidjson;

namespace ClassesRegister
{
    static std::string FormatAttributeParam(const std::string &paramStr)
    {

        if (paramStr == "true") return "EP_PARAM_BOOL(true)";
        if (paramStr == "false") return "EP_PARAM_BOOL(false)";

        {
            size_t start = 0;
            if (!paramStr.empty() && (paramStr[0] == '-' || paramStr[0] == '+')) start = 1;
            bool isInt = (start < paramStr.size());
            for (size_t i = start; i < paramStr.size() && isInt; ++i)
                if (!std::isdigit((unsigned char) paramStr[i])) isInt = false;
            if (isInt)
                return "EP_PARAM_INT(" + paramStr + ")";
        }

        {
            size_t start = 0;
            if (!paramStr.empty() && (paramStr[0] == '-' || paramStr[0] == '+')) start = 1;
            size_t dotPos = paramStr.find('.');
            if (dotPos != std::string::npos && dotPos > start && dotPos + 1 < paramStr.size())
            {
                bool isFloat = true;
                for (size_t i = start; i < paramStr.size() && isFloat; ++i)
                {
                    if (i == dotPos) continue;
                    if (!std::isdigit((unsigned char) paramStr[i])) isFloat = false;
                }
                if (isFloat)
                    return "EP_PARAM_FLOAT(" + paramStr + "f)";
            }
        }

        std::string escaped;
        for (char c: paramStr)
        {
            if (c == '"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else escaped += c;
        }
        return "EP_PARAM_STRING(\"" + escaped + "\")";
    }

    static std::string FormatAttrList(const std::vector<AttributeInstance> &attrs)
    {
        std::string s = "{";
        bool first = true;
        for (const auto &a: attrs)
        {
            if (!a.name.empty())
            {
                if (!first) s += ", ";
                s += "AttributeInfo{\"";
                s += a.MacroName();
                s += "\", {";
                bool firstParam = true;
                for (const auto &p: a.params)
                {
                    if (!firstParam) s += ", ";
                    s += FormatAttributeParam(p);
                    firstParam = false;
                }
                s += "}}";
                first = false;
            }
        }
        s += "}";
        return s;
    }

    static bool ShouldRegisterField(const FieldInfo &field)
    {
        return !field.HasAttribute(NoReflectableHandler::kKind);
    }

    static std::string GenerateRegisterCodeString(const TargetClassDetail &detail, int &registeredCount)
    {
        std::ostringstream ofs;

        ofs << "//auto generate\n";
        ofs << "#include \"Reflection.h\"\n";
        ofs << "using namespace attrfl;\n";
        std::unordered_set<std::string> includeSet;
        for (const ClassInfo &classInfo: detail.classes)
        {
            auto include = classInfo.includes[0];
            if (classInfo.reflectable && includeSet.insert(include).second)
                ofs << "#include \"" << include << "\"\n";
        }
        ofs << "\n";

        ofs << "namespace{\n";
        ofs << "struct __reflect_register__" << detail.target << "\n";
        ofs << "{\n";
        ofs << "    __reflect_register__" << detail.target << "()\n";
        ofs << "    {\n";
        for (const ClassInfo &classInfo: detail.classes)
        {
            if (!classInfo.reflectable) continue;
            registeredCount++;

            if (classInfo.bases.empty())
            {
                ofs << "        Reflect::RegisterType<" << classInfo.qualifiedName
                        << ">(\"" << classInfo.name << "\");\n";
            }
            else
            {
                ofs << "        Reflect::RegisterType<" << classInfo.qualifiedName;
                for (const std::string &base: classInfo.bases)
                    ofs << ", " << base;
                ofs << ">(\"" << classInfo.name << "\");\n";
            }

            if (!classInfo.attributes.empty())
            {
                ofs << "        Type<" << classInfo.qualifiedName << ">::SetAttributes("
                        << FormatAttrList(classInfo.attributes) << ");\n";
            }

            bool hasFields = false;
            for (const auto &field: classInfo.fields)
            {
                if (ShouldRegisterField(field))
                {
                    hasFields = true;
                    break;
                }
            }
            if (hasFields)
                ofs << "        " << classInfo.qualifiedName << "::__RegisterField();\n";
        }
        ofs << "    }\n";
        ofs << "};\n";
        ofs << "static __reflect_register__" << detail.target << " __reflect;\n";
        ofs << "}\n";

        for (const ClassInfo &classInfo: detail.classes)
        {
            if (!classInfo.reflectable) continue;

            std::vector<const FieldInfo *> fieldsToRegister;
            for (const auto &field: classInfo.fields)
            {
                if (ShouldRegisterField(field))
                    fieldsToRegister.push_back(&field);
            }
            if (fieldsToRegister.empty()) continue;

            ofs << "\nvoid " << classInfo.qualifiedName << "::__RegisterField()\n";
            ofs << "{\n";
            for (const auto *field: fieldsToRegister)
            {
                ofs << "    Type<" << classInfo.qualifiedName << ">::RegisterField(\""
                        << field->name << "\", \""
                        << field->access << "\", &"
                        << classInfo.qualifiedName << "::" << field->name << ", "
                        << FormatAttrList(field->attributes) << ");\n";
            }
            ofs << "}\n";
        }

        return ofs.str();
    }
}
