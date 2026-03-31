#pragma once

#include <clang-c/Index.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ClassInfo.h"
#include "Attributes/AttributeRegistry.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/ostreamwrapper.h"
#include "Generic.h"

namespace fs = std::filesystem;

namespace CollectClasses
{
    static std::string ToString(CXString s)
    {
        const char *c = clang_getCString(s);
        std::string out = c ? c : "";
        clang_disposeString(s);
        return out;
    }

    static std::vector<std::string> SplitSeparators(const std::string &s)
    {
        std::vector<std::string> out;
        std::string cur;
        auto push_cur = [&]()
        {
            size_t a = 0;
            while (a < cur.size() && std::isspace((unsigned char) cur[a])) ++a;
            size_t b = cur.size();
            while (b > a && std::isspace((unsigned char) cur[b - 1])) --b;
            if (b > a) out.push_back(cur.substr(a, b - a));
            cur.clear();
        };
        for (size_t i = 0; i < s.size(); ++i)
        {
            char ch = s[i];
            if (ch == '\n' || ch == ';' || ch == '\r')
            {
                push_cur();
            }
            else
            {
                cur.push_back(ch);
            }
        }
        push_cur();
        return out;
    }

    static std::vector<std::string> ReadAndSplit(const fs::path &p)
    {
        std::ifstream ifs(p, std::ios::binary);
        if (!ifs) throw std::runtime_error("Cannot open file: " + p.string());
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return SplitSeparators(content);
    }

    static std::string CursorSpelling(CXCursor c)
    {
        return ToString(clang_getCursorSpelling(c));
    }

    static std::string GetFilePath(CXSourceLocation loc, unsigned *outLine)
    {
        CXFile file;
        unsigned line = 0, col = 0, offset = 0;
        clang_getSpellingLocation(loc, &file, &line, &col, &offset);
        if (outLine) *outLine = line;
        if (!file) return "";
        return ToString(clang_getFileName(file));
    }

    static bool IsInterestingClassCursorKind(CXCursorKind k)
    {
        switch (k)
        {
            case CXCursor_ClassDecl:
            case CXCursor_StructDecl:
            case CXCursor_UnionDecl:
            case CXCursor_ClassTemplate:
                return true;
            default:
                return false;
        }
    }

    static std::string BuildNamespaceOnly(CXCursor c)
    {
        std::vector<std::string> parts;
        CXCursor cur = clang_getCursorSemanticParent(c);
        while (!clang_Cursor_isNull(cur) && clang_getCursorKind(cur) != CXCursor_TranslationUnit)
        {
            CXCursorKind k = clang_getCursorKind(cur);
            if (k == CXCursor_Namespace)
            {
                auto name = CursorSpelling(cur);
                if (!name.empty()) parts.push_back(name);
            }
            cur = clang_getCursorSemanticParent(cur);
        }
        std::reverse(parts.begin(), parts.end());
        std::ostringstream oss;
        for (size_t i = 0; i < parts.size(); i++)
        {
            if (i) oss << "::";
            oss << parts[i];
        }
        return oss.str();
    }

    static std::string BuildQualifiedName(CXCursor c)
    {
        std::vector<std::string> scopes;
        CXCursor cur = c;
        while (!clang_Cursor_isNull(cur) && clang_getCursorKind(cur) != CXCursor_TranslationUnit)
        {
            CXCursorKind k = clang_getCursorKind(cur);
            if (k == CXCursor_Namespace || k == CXCursor_ClassDecl || k == CXCursor_StructDecl || k ==
                CXCursor_UnionDecl ||
                k == CXCursor_ClassTemplate)
            {
                auto name = CursorSpelling(cur);
                if (!name.empty()) scopes.push_back(name);
            }
            cur = clang_getCursorSemanticParent(cur);
        }
        std::reverse(scopes.begin(), scopes.end());
        std::ostringstream oss;
        for (size_t i = 0; i < scopes.size(); i++)
        {
            if (i) oss << "::";
            oss << scopes[i];
        }
        return oss.str();
    }

    struct VisitContext
    {
        std::vector<ClassInfo> *classes = nullptr;
        std::unordered_set<std::string> *seen = nullptr;
        std::vector<std::string> *allowedRoots = nullptr;
        std::vector<std::string> *includeDirs = nullptr;
    };

    static std::string NormalizePathForCompare(const std::string &p)
    {
        if (p.empty()) return std::string();
        try
        {
            fs::path pp(p);
            if (!pp.is_absolute()) pp = fs::absolute(pp);
            std::string s = pp.lexically_normal().generic_string();
#ifdef _WIN32
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
#endif
            return s;
        }
        catch (...)
        {
            std::string s = p;
            for (auto &ch: s) if (ch == '\\') ch = '/';
#ifdef _WIN32
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
#endif
            return s;
        }
    }

    static std::string ResolveTypeName(CXType ft)
    {
        return ToString(clang_getTypeSpelling(ft));
    }

    static bool PathHasPrefix(const std::string &path_norm, const std::string &prefix_norm)
    {
        if (prefix_norm.empty()) return false;
        if (path_norm.size() < prefix_norm.size()) return false;
        if (path_norm.compare(0, prefix_norm.size(), prefix_norm) == 0)
        {
            if (path_norm.size() == prefix_norm.size()) return true;
            char c = path_norm[prefix_norm.size()];
            return c == '/';
        }
        return false;
    }

    static std::vector<std::string> ComputeIncludeSuggestions(const std::string &fileNorm,
                                                                const std::vector<std::string> &includeDirs)
    {
        std::vector<std::string> res;
        for (const auto &inc: includeDirs)
        {
            if (inc.empty()) continue;
            if (PathHasPrefix(fileNorm, inc))
            {
                std::string rel = fileNorm.substr(inc.size());
                if (!rel.empty() && rel[0] == '/') rel.erase(0, 1);
                if (!rel.empty()) res.push_back(rel);
            }
        }
        std::vector<std::string> out;
        std::unordered_set<std::string> seen;
        for (auto &r: res)
        {
            if (seen.insert(r).second) out.push_back(r);
        }
        return out;
    }

    static void CollectHeadersFromDirs(const std::vector<std::string> &dirs,
                                          std::vector<std::string> &out,
                                          const std::vector<std::string> &patterns = {
                                              ".h", ".hpp", ".hh", ".hxx", ".inl"
                                          })
    {
        std::unordered_set<std::string> seen;
        std::error_code ec;
        for (const auto &d_raw: dirs)
        {
            if (d_raw.empty()) continue;
            fs::path root(d_raw);
            if (!root.is_absolute())
            {
                root = fs::absolute(root, ec);
            }
            if (!fs::exists(root, ec)) continue;
            for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec);
                 it != fs::recursive_directory_iterator(); ++it)
            {
                if (ec) break;
                std::error_code ec2;
                if (!fs::is_regular_file(it->path(), ec2) || ec2) continue;
                std::string ext = it->path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
                bool ok = false;
                for (auto &p: patterns)
                    if (ext == p)
                    {
                        ok = true;
                        break;
                    }
                if (!ok) continue;
                std::string abs = it->path().lexically_normal().generic_string();
#ifdef _WIN32
                std::transform(abs.begin(), abs.end(), abs.begin(), [](unsigned char c) { return std::tolower(c); });
#endif
                if (seen.insert(abs).second) out.push_back(abs);
            }
        }
    }

    static CXChildVisitResult Visitor(CXCursor cursor, CXCursor /*parent*/, CXClientData client_data)
    {
        auto *ctx = reinterpret_cast<VisitContext *>(client_data);
        CXCursorKind k = clang_getCursorKind(cursor);

        if (IsInterestingClassCursorKind(k))
        {
            if (!clang_isCursorDefinition(cursor)) return CXChildVisit_Recurse;

            std::string name = CursorSpelling(cursor);
            if (name.empty()) return CXChildVisit_Recurse;

            unsigned line = 0;
            std::string file = GetFilePath(clang_getCursorLocation(cursor), &line);
            if (file.empty()) return CXChildVisit_Recurse;

            std::string fileNorm;
            bool isExternal = false;
            if (ctx->allowedRoots && !ctx->allowedRoots->empty())
            {
                fileNorm = NormalizePathForCompare(file);
                bool allowed = false;
                for (const auto &pref: *ctx->allowedRoots)
                {
                    if (PathHasPrefix(fileNorm, pref))
                    {
                        allowed = true;
                        break;
                    }
                }
                if (!allowed) isExternal = true;
            }
            else
            {
                fileNorm = NormalizePathForCompare(file);
            }

            bool reflectable = false;

            ClassInfo info;
            info.name = name;
            info.ns = BuildNamespaceOnly(cursor);
            info.qualifiedName = BuildQualifiedName(cursor);

            struct ChildCtx
            {
                bool *reflectable;
                ClassInfo *info;
                bool *isExternal;
            };
            ChildCtx childCtx{&reflectable, &info, &isExternal};

            clang_visitChildren(cursor,
                                [](CXCursor c, CXCursor /*parent*/, CXClientData cd) -> CXChildVisitResult
                                {
                                    auto *ctx = static_cast<ChildCtx *>(cd);
                                    CXCursorKind ck = clang_getCursorKind(c);

                                    if (ck == CXCursor_CXXBaseSpecifier)
                                    {
                                        CXType baseType = clang_getCursorType(c);
                                        std::string baseSpelling = ToString(clang_getTypeSpelling(baseType));

                                        CXCursor baseDecl = clang_getTypeDeclaration(baseType);
                                        if (!clang_Cursor_isNull(baseDecl) && clang_getCursorKind(baseDecl) !=
                                            CXCursor_NoDeclFound)
                                        {
                                            std::string qual = BuildQualifiedName(baseDecl);
                                            if (!qual.empty()) baseSpelling = qual;
                                        }

                                        if (!baseSpelling.empty())
                                        {
                                            if (std::find(ctx->info->bases.begin(), ctx->info->bases.end(),
                                                          baseSpelling) == ctx->info->bases.end())
                                            {
                                                ctx->info->bases.push_back(baseSpelling);
                                            }
                                        }
                                        return CXChildVisit_Recurse;
                                    }

                                    if (ck == CXCursor_AnnotateAttr)
                                    {
                                        std::string anno = ToString(clang_getCursorSpelling(c));
                                        if (anno.rfind("class:", 0) == 0)
                                        {
                                            *ctx->reflectable = true;
                                            for (auto &attr: ParseAttributeTokens(anno.substr(6)))
                                            {
                                                AttributeHandler *handler = GetAttributeHandler(attr.kind);
                                                if (handler) handler->OnClass(*ctx->info, attr);
                                            }
                                        }
                                        return CXChildVisit_Continue;
                                    }

                                    if (ck == CXCursor_FieldDecl)
                                    {
                                        if (*ctx->isExternal) return CXChildVisit_Continue;
                                        FieldInfo fi;
                                        fi.name = ToString(clang_getCursorSpelling(c));
                                        fi.type = ResolveTypeName(clang_getCursorType(c));
                                        switch (clang_getCXXAccessSpecifier(c))
                                        {
                                            case CX_CXXPublic: fi.access = "public";
                                                break;
                                            case CX_CXXProtected: fi.access = "protected";
                                                break;
                                            case CX_CXXPrivate: fi.access = "private";
                                                break;
                                            default: fi.access = "public";
                                                break;
                                        }

                                        struct FldCtx
                                        {
                                            FieldInfo *field;
                                            bool *reflectable;
                                        };
                                        FldCtx fctx{&fi, ctx->reflectable};
                                        clang_visitChildren(c,
                                                            [](CXCursor cc, CXCursor,
                                                               CXClientData cd2) -> CXChildVisitResult
                                                            {
                                                                auto *fc = static_cast<FldCtx *>(cd2);
                                                                if (clang_getCursorKind(cc) == CXCursor_AnnotateAttr)
                                                                {
                                                                    std::string anno = ToString(
                                                                        clang_getCursorSpelling(cc));
                                                                    if (anno.rfind("field:", 0) == 0)
                                                                    {
                                                                        *fc->reflectable = true;
                                                                        for (auto &attr: ParseAttributeTokens(
                                                                                 anno.substr(6)))
                                                                        {
                                                                            AttributeHandler *handler =
                                                                                    GetAttributeHandler(attr.kind);
                                                                            if (handler)
                                                                                handler->OnField(
                                                                                    *fc->field, attr);
                                                                        }
                                                                    }
                                                                }
                                                                return CXChildVisit_Continue;
                                                            }, &fctx);


                                        ctx->info->fields.push_back(std::move(fi));
                                        return CXChildVisit_Continue;
                                    }

                                    if (IsInterestingClassCursorKind(ck))
                                        return CXChildVisit_Continue;

                                    return CXChildVisit_Recurse;
                                }, &childCtx);

            info.reflectable = reflectable;
            info.external = isExternal;

            if (ctx->includeDirs && !ctx->includeDirs->empty())
            {
                info.includes = ComputeIncludeSuggestions(fileNorm, *ctx->includeDirs);
            }
            if (info.includes.empty())
            {
                fs::path p(file);
                std::string fallback = p.lexically_normal().generic_string();
                info.includes.push_back(fallback);
            }

            CXString usr_cx = clang_getCursorUSR(cursor);
            std::string usr = ToString(usr_cx);
            std::string key;
            if (!usr.empty())
            {
                key = usr;
            }
            else
            {
                key = info.qualifiedName + "|" + file + "|" + std::to_string(line);
            }
            if (ctx->seen->insert(key).second)
            {
                ctx->classes->push_back(std::move(info));
            }
        }

        return CXChildVisit_Recurse;
    }

    static void PropagateClassAttributes(std::vector<ClassInfo> &classes)
    {
        std::unordered_map<std::string, ClassInfo *> nameToInfo;
        for (auto &cls: classes)
            nameToInfo[cls.qualifiedName] = &cls;

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto &cls: classes)
            {
                for (const auto &baseName: cls.bases)
                {
                    auto it = nameToInfo.find(baseName);
                    if (it == nameToInfo.end()) continue;
                    const ClassInfo *base = it->second;

                    for (AttributeKind kind: GetAllKinds())
                    {
                        if (cls.HasAttribute(kind)) continue;
                        const AttributeInstance *baseAttr = base->FindAttribute(kind);
                        if (!baseAttr) continue;
                        AttributeHandler *handler = GetAttributeHandler(kind);
                        if (!handler || !handler->ShouldPropagate(*baseAttr)) continue;

                        AttributeKind finalKind = GetFinalKindFor(kind);
                        if (finalKind != AttributeKind::Unknown && base->HasAttribute(finalKind))
                            continue;

                        if (finalKind != AttributeKind::Unknown)
                        {
                            const AttributeInstance *finalAttr = cls.FindAttribute(finalKind);
                            if (finalAttr)
                            {
                                if (ParseBoolParam(*finalAttr, 0))
                                {
                                    cls.attributes.push_back(*baseAttr);
                                    cls.reflectable = true;
                                    changed = true;
                                }
                                continue;
                            }
                        }

                        cls.attributes.push_back(*baseAttr);
                        cls.reflectable = true;
                        changed = true;
                    }
                }
            }
        }
    }

    static void CheckMultiInheritConflicts(const std::vector<ClassInfo> &classes)
    {
        std::unordered_map<std::string, const ClassInfo *> nameToInfo;
        for (const auto &cls: classes)
            nameToInfo[cls.qualifiedName] = &cls;

        for (const auto &cls: classes)
        {
            if (cls.bases.size() < 2) continue;

            std::unordered_map<int, std::string> groupOwner;

            for (const auto &baseName: cls.bases)
            {
                auto it = nameToInfo.find(baseName);
                if (it == nameToInfo.end()) continue;
                const ClassInfo *base = it->second;

                for (AttributeKind kind: GetAllKinds())
                {
                    int gid = GetConflictGroupId(kind);
                    if (gid == 0) continue;

                    const AttributeInstance *attr = base->FindAttribute(kind);
                    if (!attr) continue;

                    AttributeHandler *h = GetAttributeHandler(kind);
                    bool propagates = h && h->ShouldPropagate(*attr);

                    if (!propagates && !IsFinalKind(kind)) continue;

                    auto existing = groupOwner.find(gid);
                    if (existing != groupOwner.end() && existing->second != baseName)
                    {
                        std::cerr << "[attrfl][conflict] class '" << cls.qualifiedName
                                << "' has conflict in " << GetConflictGroupName(gid)
                                << " group: base '" << existing->second << "' vs '"
                                << baseName << "'\n";
                        throw std::runtime_error(
                            "Multi-inheritance attribute conflict in '" + cls.qualifiedName + "'");
                    }
                    groupOwner[gid] = baseName;
                }
            }
        }
    }

    static void ApplyFieldPriorities(std::vector<ClassInfo> &classes)
    {
        for (auto &cls: classes)
        {
            for (auto &field: cls.fields)
            {
                auto attrsCopy = field.attributes;
                for (const auto &attr: attrsCopy)
                {
                    AttributeHandler *h = GetAttributeHandler(attr.kind);
                    if (h) h->OnPostField(field, attr);
                }
            }
        }
    }

    static void StripNoAttributes(std::vector<ClassInfo> &classes)
    {
        for (auto &cls: classes)
        {
            for (auto &field: cls.fields)
            {
                field.attributes.erase(
                    std::remove_if(field.attributes.begin(), field.attributes.end(),
                                   [](const AttributeInstance &a)
                                   {
                                       AttributeHandler *h = GetAttributeHandler(a.kind);
                                       return h && !h->IsPositiveForReflect();
                                   }),
                    field.attributes.end()
                );
            }
        }
    }

    static void ExpandClassAttributesToFields(std::vector<ClassInfo> &classes)
    {
        for (auto &cls: classes)
        {
            if (!cls.reflectable) continue;

            std::vector<AttributeKind> impliedKinds;
            for (const auto &clsAttr: cls.attributes)
            {
                AttributeHandler *h = GetAttributeHandler(clsAttr.kind);
                if (!h || !h->ContributesFieldCollect(clsAttr)) continue;

                AttributeKind impliedKind = h->GetImpliedFieldKind();
                if (impliedKind == AttributeKind::Unknown) continue;

                bool already = false;
                for (auto k: impliedKinds) if (k == impliedKind)
                {
                    already = true;
                    break;
                }
                if (!already) impliedKinds.push_back(impliedKind);
            }

            if (impliedKinds.empty()) continue;

            for (auto &field: cls.fields)
            {
                for (AttributeKind impliedKind: impliedKinds)
                {
                    if (!field.HasAttribute(impliedKind))
                    {
                        AttributeInstance implied;
                        implied.kind = impliedKind;
                        implied.name = AttributeInternalName(impliedKind);
                        field.attributes.push_back(implied);
                    }
                }
            }
        }
    }

    static std::vector<ClassInfo> FilterProcessedClasses(const std::vector<ClassInfo> &classes)
    {
        std::vector<ClassInfo> result;
        for (const auto &cls: classes)
        {
            if (!cls.reflectable) continue;
            if (cls.external) continue;
            ClassInfo filtered = cls;
            filtered.fields.clear();
            for (const auto &field: cls.fields)
            {
                if (!field.attributes.empty())
                    filtered.fields.push_back(field);
            }
            result.push_back(std::move(filtered));
        }
        return result;
    }

    static void WriteJsonSimple(const fs::path &outPath, const std::string &targetName,
                                  const std::vector<ClassInfo> &classes)
    {
        fs::create_directories(outPath.parent_path());
        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open output file: " + outPath.string());


        OStreamWrapper osw(ofs);
        PrettyWriter<OStreamWrapper> writer(osw);
        TargetClassDetail detail;
        detail.target = targetName;
        detail.generatedAt = GetTimestamp();
        detail.classes = classes;
        detail.Serialize(writer);
    }
}
