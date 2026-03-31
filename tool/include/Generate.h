#pragma once

#include "CollectClasses.h"
#include "ClassesRegister.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/document.h"
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace Generate
{
    static std::string HashString(const std::string &s)
    {
        size_t h = std::hash<std::string>{}(s);

        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        std::ostringstream oss;
        oss << std::hex << h;
        return oss.str();
    }

    static std::string HashFileContent(const fs::path &path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return "";
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return HashString(content);
    }

    static std::string MtimeToString(const fs::file_time_type &t)
    {
        return std::to_string(t.time_since_epoch().count());
    }

    struct StampData
    {
        std::string flagsHash;
        std::string includeHash;
        std::string depsHash;
        std::unordered_map<std::string, std::string> fileMtimes;
    };

    static StampData ReadStamp(const fs::path &stampPath)
    {
        StampData data;
        std::ifstream ifs(stampPath);
        if (!ifs) return data;

        std::string line;
        bool inFiles = false;
        while (std::getline(ifs, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line == "files:")
            {
                inFiles = true;
                continue;
            }
            if (inFiles)
            {
                auto eq = line.find('=');
                if (eq != std::string::npos)
                    data.fileMtimes[line.substr(0, eq)] = line.substr(eq + 1);
            }
            else
            {
                if (line.rfind("flags_hash=", 0) == 0)
                    data.flagsHash = line.substr(11);
                else if (line.rfind("include_hash=", 0) == 0)
                    data.includeHash = line.substr(13);
                else if (line.rfind("deps_hash=", 0) == 0)
                    data.depsHash = line.substr(10);
            }
        }
        return data;
    }

    static void WriteStamp(const fs::path &stampPath,
                           const std::string &flagsHash,
                           const std::string &includeHash,
                           const std::string &depsHash,
                           const std::unordered_map<std::string, std::string> &fileMtimes)
    {
        fs::create_directories(stampPath.parent_path());
        std::ofstream ofs(stampPath);
        if (!ofs) throw std::runtime_error("Cannot open stamp file: " + stampPath.string());
        ofs << "flags_hash=" << flagsHash << "\n";
        ofs << "include_hash=" << includeHash << "\n";
        ofs << "deps_hash=" << depsHash << "\n";
        ofs << "files:\n";
        for (const auto &kv: fileMtimes)
            ofs << kv.first << "=" << kv.second << "\n";
    }

    static fs::path OrgJsonPath(const fs::path &orgDir, const std::string &headerNorm)
    {
        return orgDir / (HashString(headerNorm) + ".json");
    }

    static void WriteOrgJson(const fs::path &orgPath, const std::string &targetName,
                             const std::vector<ClassInfo> &classes)
    {
        CollectClasses::WriteJsonSimple(orgPath, targetName, classes);
    }

    static std::vector<ClassInfo> LoadAllOrgJsons(const fs::path &orgDir)
    {
        std::vector<ClassInfo> allClasses;
        if (!fs::exists(orgDir)) return allClasses;

        std::error_code ec;
        for (auto &entry: fs::directory_iterator(orgDir, ec))
        {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".json") continue;

            try
            {
                std::ifstream ifs(entry.path(), std::ios::binary);
                if (!ifs) continue;
                rapidjson::IStreamWrapper isw(ifs);
                rapidjson::Document doc;
                if (doc.ParseStream(isw).HasParseError()) continue;

                TargetClassDetail detail;
                detail.Deserialize(doc);
                for (auto &cls: detail.classes)
                    allClasses.push_back(std::move(cls));
            }
            catch (...)
            {
            }
        }

        return allClasses;
    }

    static std::vector<std::string> SplitPipe(const std::string &s)
    {
        std::vector<std::string> result;
        size_t pos = 0;
        while (pos <= s.size())
        {
            size_t nxt = s.find('|', pos);
            std::string piece = (nxt == std::string::npos) ? s.substr(pos) : s.substr(pos, nxt - pos);

            size_t a = 0, b = piece.size();
            while (a < b && isspace((unsigned char) piece[a])) ++a;
            while (b > a && isspace((unsigned char) piece[b - 1])) --b;
            if (b > a) result.push_back(piece.substr(a, b - a));
            if (nxt == std::string::npos) break;
            pos = nxt + 1;
        }
        return result;
    }

    static std::vector<ClassInfo> LoadDepsClasses(const std::string &depsArg)
    {
        std::vector<ClassInfo> result;
        auto paths = SplitPipe(depsArg);
        for (auto &p: paths)
        {
            if (p.empty()) continue;
            fs::path jsonPath = p;
            if (!fs::exists(jsonPath))
            {
                std::cerr << "[attrfl][deps][warn] dep json not found, skip: " << p << "\n";
                continue;
            }
            try
            {
                std::ifstream ifs(jsonPath, std::ios::binary);
                if (!ifs)
                {
                    std::cerr << "[attrfl][deps][warn] cannot open: " << p << "\n";
                    continue;
                }
                rapidjson::IStreamWrapper isw(ifs);
                rapidjson::Document doc;
                if (doc.ParseStream(isw).HasParseError())
                {
                    std::cerr << "[attrfl][deps][warn] json parse failed: " << p << "\n";
                    continue;
                }
                TargetClassDetail detail;
                detail.Deserialize(doc);
                size_t count = detail.classes.size();
                for (auto &cls: detail.classes)
                {
                    cls.external = true;
                    result.push_back(std::move(cls));
                }
            }
            catch (const std::exception &ex)
            {
                std::cerr << "[attrfl][deps][warn] load error " << p << ": " << ex.what() << "\n";
            }
        }
        return result;
    }

    static std::string HashDeps(const std::string &depsArg)
    {
        if (depsArg.empty()) return "";
        auto paths = SplitPipe(depsArg);
        std::string combined;
        for (auto &p: paths)
        {
            if (p.empty()) continue;
            std::error_code ec;
            auto t = fs::last_write_time(p, ec);
            if (!ec)
                combined += p + "=" + MtimeToString(t) + ";";
            else
                combined += p + "=missing;";
        }
        return HashString(combined);
    }

    static bool WriteIfChanged(const fs::path &path, const std::string &newContent)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (ifs)
        {
            std::string existing((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            if (existing == newContent) return false;
        }

        fs::create_directories(path.parent_path());
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open output file: " + path.string());
        ofs << newContent;
        return true;
    }

    struct ParseResult
    {
        std::string headerPath;
        std::string headerNorm;
        std::vector<ClassInfo> classes;
        long long ms;
    };


    static std::vector<ClassInfo> ParseSingleHeader(CXIndex index,
                                                    const std::string &headerPath,
                                                    const std::vector<std::string> &normFlags,
                                                    const std::vector<std::string> &includeDirsNorm)
    {
        std::string headerNorm = CollectClasses::NormalizePathForCompare(headerPath);
        std::vector<std::string> singleFileRoot = {headerNorm};

        std::vector<ClassInfo> classes;
        std::unordered_set<std::string> seen;

        std::vector<std::string> argStorage = normFlags;
        argStorage.emplace_back("-x");
        argStorage.emplace_back("c++");

        std::vector<const char *> args;
        args.reserve(argStorage.size());
        for (auto &a: argStorage) args.push_back(a.c_str());

        unsigned options =
                CXTranslationUnit_SkipFunctionBodies;

        CXTranslationUnit tu = clang_parseTranslationUnit(
            index, headerPath.c_str(),
            args.empty() ? nullptr : args.data(),
            static_cast<int>(args.size()),
            nullptr, 0, options);

        if (!tu)
        {
            std::cerr << "[attrfl][clang][error] failed to parse: " << headerPath << "\n";
            return {};
        }

        unsigned numDiag = clang_getNumDiagnostics(tu);
        for (unsigned i = 0; i < numDiag; i++)
        {
            CXDiagnostic diag = clang_getDiagnostic(tu, i);
            if (clang_getDiagnosticSeverity(diag) >= CXDiagnostic_Error)
            {
                std::string diagStr = CollectClasses::ToString(
                    clang_formatDiagnostic(diag, CXDiagnostic_DisplaySourceLocation));
                if (diagStr.find("file not found") == std::string::npos)
                {
                    std::cerr << "[attrfl][diag] " << diagStr << "\n";
                }
            }
            clang_disposeDiagnostic(diag);
        }

        CXCursor root = clang_getTranslationUnitCursor(tu);

        std::vector<std::string> includeDirsLocal = includeDirsNorm;
        CollectClasses::VisitContext ctx;
        ctx.classes = &classes;
        ctx.seen = &seen;
        ctx.allowedRoots = &singleFileRoot;
        ctx.includeDirs = includeDirsLocal.empty() ? nullptr : &includeDirsLocal;
        clang_visitChildren(root, CollectClasses::Visitor, &ctx);
        clang_disposeTranslationUnit(tu);

        std::vector<ClassInfo> result;
        for (auto &cls: classes)
            if (!cls.external)
                result.push_back(std::move(cls));
        return result;
    }

    static void SaveDebugParams(const fs::path &outDir, const std::string &targetName, int argc, char **argv)
    {
        std::string fileName = targetName.empty()
                                   ? "attrfl_params.txt"
                                   : "attrfl_" + targetName + "_params.txt";

        fs::path txtPath = outDir / fileName;

        std::error_code ec;
        fs::create_directories(outDir, ec);

        std::ofstream ofs(txtPath);
        if (!ofs)
        {
            std::cerr << "[attrfl][debug] cannot write params file: " << txtPath << "\n";
            return;
        }

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--generate" || arg == "--debug") continue;
            ofs << arg << "\n";
        }
    }

    static int GenerateDebug(int argc, char **argv);

    static int Generate(int argc, char **argv)
    {
        try
        {
            auto startTime = std::chrono::steady_clock::now();

            auto includePathOpt = GetArgValue(argc, argv, "--include_path");
            auto flagsPathOpt = GetArgValue(argc, argv, "--flags");
            auto outJsonOpt = GetArgValue(argc, argv, "--out");
            auto outCppOpt = GetArgValue(argc, argv, "--out_cpp");
            auto targetOpt = GetArgValue(argc, argv, "--target");
            auto rootsOpt = GetArgValue(argc, argv, "--roots");
            auto stampOpt = GetArgValue(argc, argv, "--stamp");
            auto depsOpt = GetArgValue(argc, argv, "--deps");
            bool force = HasFlag(argc, argv, "--force");

            std::string targetName = targetOpt ? *targetOpt : "";
            std::string depsArg = depsOpt ? *depsOpt : "";

            if (outJsonOpt)
                SaveDebugParams(fs::path(*outJsonOpt).parent_path(), targetName, argc, argv);

            std::cerr << "[attrfl][startup] Generate (" << targetName << ") starting\n";

            if (!includePathOpt || !outJsonOpt || !outCppOpt || !stampOpt)
            {
                std::cerr << "[attrfl][error] missing required args (--include_path/--out/--out_cpp/--stamp)\n";
                return 2;
            }

            fs::path stampPath = *stampOpt;
            fs::path outJsonPath = *outJsonOpt;
            fs::path outCppPath = *outCppOpt;

            fs::path orgDir = outJsonPath.parent_path() / "org";

            std::vector<std::string> includeDirs;
            {
                fs::path incp = *includePathOpt;
                includeDirs = CollectClasses::ReadAndSplit(incp);
            }

            std::vector<std::string> flags;
            if (flagsPathOpt)
                flags = CollectClasses::ReadAndSplit(*flagsPathOpt);

            std::vector<std::string> allowedRootsRaw;
            if (rootsOpt)
            {
                const std::string &r = *rootsOpt;
                size_t pos = 0;
                while (pos < r.size())
                {
                    size_t nxt = r.find('|', pos);
                    std::string piece = (nxt == std::string::npos) ? r.substr(pos) : r.substr(pos, nxt - pos);
                    size_t a = 0, b = piece.size();
                    while (a < piece.size() && isspace((unsigned char) piece[a])) ++a;
                    while (b > a && isspace((unsigned char) piece[b - 1])) --b;
                    if (b > a) allowedRootsRaw.push_back(piece.substr(a, b - a));
                    if (nxt == std::string::npos) break;
                    pos = nxt + 1;
                }
            }

            std::vector<std::string> allowedRootsNorm;
            for (auto &p: allowedRootsRaw)
            {
                std::string n = CollectClasses::NormalizePathForCompare(p);
                if (!n.empty()) allowedRootsNorm.push_back(n);
            }

            std::vector<std::string> currentHeaders;
            if (!allowedRootsRaw.empty())
                CollectClasses::CollectHeadersFromDirs(allowedRootsRaw, currentHeaders);
            else
                CollectClasses::CollectHeadersFromDirs(includeDirs, currentHeaders);

            if (!allowedRootsNorm.empty())
            {
                std::vector<std::string> filtered;
                for (auto &h: currentHeaders)
                {
                    std::string hNorm = CollectClasses::NormalizePathForCompare(h);
                    for (auto &r: allowedRootsNorm)
                    {
                        if (CollectClasses::PathHasPrefix(hNorm, r))
                        {
                            filtered.push_back(h);
                            break;
                        }
                    }
                }
                currentHeaders = std::move(filtered);
            }
            std::sort(currentHeaders.begin(), currentHeaders.end());

            std::string flagsHash = flagsPathOpt ? HashFileContent(*flagsPathOpt) : "";
            std::string includeHash = includePathOpt ? HashFileContent(*includePathOpt) : "";
            std::string depsHash = HashDeps(depsArg);

            StampData lastStamp = ReadStamp(stampPath);

            bool fullRebuild = false;
            if (force)
            {
                std::cerr << "[attrfl][force] forced full rebuild\n";
                fullRebuild = true;
            }
            else
            {
                if (flagsHash != lastStamp.flagsHash)
                {
                    std::cerr << "[attrfl][change] flags changed, full rebuild\n";
                    fullRebuild = true;
                }
                if (includeHash != lastStamp.includeHash)
                {
                    std::cerr << "[attrfl][change] include dirs changed, full rebuild\n";
                    fullRebuild = true;
                }
            }

            if (fullRebuild)
            {
                std::error_code ec;
                if (fs::exists(orgDir))
                {
                    for (auto &entry: fs::directory_iterator(orgDir, ec))
                    {
                        if (!ec && entry.path().extension() == ".json")
                            fs::remove(entry.path(), ec);
                    }
                }
            }

            std::unordered_map<std::string, std::string> currentMtimes;
            for (auto &h: currentHeaders)
            {
                std::error_code ec;
                auto t = fs::last_write_time(h, ec);
                if (!ec) currentMtimes[h] = MtimeToString(t);
            }

            std::vector<std::string> toReparse;
            std::vector<std::string> toDelete;

            if (fullRebuild)
            {
                toReparse = currentHeaders;
            }
            else
            {
                std::unordered_set<std::string> currentSet(currentHeaders.begin(), currentHeaders.end());

                for (auto &kv: lastStamp.fileMtimes)
                {
                    if (currentSet.find(kv.first) == currentSet.end())
                    {
                        toDelete.push_back(kv.first);
                        std::cerr << "[attrfl][change] file removed: " << kv.first << "\n";
                    }
                }

                for (auto &h: currentHeaders)
                {
                    auto it = lastStamp.fileMtimes.find(h);
                    if (it == lastStamp.fileMtimes.end())
                    {
                        toReparse.push_back(h);
                        std::cerr << "[attrfl][change] new file: " << h << "\n";
                    }
                    else if (it->second != currentMtimes[h])
                    {
                        toReparse.push_back(h);
                        std::cerr << "[attrfl][change] modified: " << h << "\n";
                    }
                }
            }

            bool depsChanged = (depsHash != lastStamp.depsHash);

            if (!force && toReparse.empty() && toDelete.empty() && !depsChanged)
            {
                std::cerr << "[attrfl][skip] no changes, skipping\n";
                return 0;
            }

            bool skipClang = (toReparse.empty() && toDelete.empty() && depsChanged);
            if (skipClang)
            {
                std::cerr << "[attrfl][info] only deps changed, skip clang, re-run post-process\n";
            }

            if (!skipClang)
            {
                for (auto &h: toDelete)
                {
                    fs::path orgPath = OrgJsonPath(orgDir, CollectClasses::NormalizePathForCompare(h));
                    std::error_code ec;
                    fs::remove(orgPath, ec);
                }

                std::vector<std::string> normFlags;
                for (auto &f: flags) { if (!f.empty()) normFlags.push_back(f); }
                for (auto &d: includeDirs) { if (!d.empty()) normFlags.push_back("-I" + d); }

                bool hasStd = false;
                for (auto &f: normFlags)
                    if (f.rfind("-std=", 0) == 0)
                    {
                        hasStd = true;
                        break;
                    }
                if (!hasStd) normFlags.insert(normFlags.begin(), "-std=c++17");

                std::vector<std::string> includeDirsNorm;
                for (auto &f: normFlags)
                {
                    if (f.rfind("-I", 0) == 0 && f.size() > 2)
                    {
                        std::string n = CollectClasses::NormalizePathForCompare(f.substr(2));
                        if (!n.empty()) includeDirsNorm.push_back(n);
                    }
                }
                std::sort(includeDirsNorm.begin(), includeDirsNorm.end());
                includeDirsNorm.erase(std::unique(includeDirsNorm.begin(), includeDirsNorm.end()),
                                      includeDirsNorm.end());

                fs::create_directories(orgDir);

                bool useMultiThread = HasFlag(argc, argv, "--MT");
                if (HasFlag(argc, argv, "--NOSTDINC"))
                    normFlags.emplace_back("-nostdinc");
                if (useMultiThread)
                {
                    size_t fileCount = toReparse.size();
                    size_t threadCount = std::max<size_t>(1,
                                                          std::min<size_t>(
                                                              std::thread::hardware_concurrency(), fileCount));
                    std::cerr << "[attrfl][info] using " << threadCount << " threads for "
                            << fileCount << " files\n";

                    std::vector<ParseResult> results(fileCount);
                    std::atomic<size_t> taskIndex{0};

                    std::vector<std::thread> threads;
                    threads.reserve(threadCount);
                    for (size_t t = 0; t < threadCount; ++t)
                    {
                        threads.emplace_back([&]()
                        {
                            CXIndex threadIndex = clang_createIndex(0, 0);
                            while (true)
                            {
                                size_t i = taskIndex.fetch_add(1, std::memory_order_relaxed);
                                if (i >= fileCount) break;

                                const std::string &h = toReparse[i];
                                auto t0 = std::chrono::steady_clock::now();
                                auto fileClasses = ParseSingleHeader(threadIndex, h, normFlags, includeDirsNorm);
                                auto t1 = std::chrono::steady_clock::now();

                                results[i].headerPath = h;
                                results[i].headerNorm = CollectClasses::NormalizePathForCompare(h);
                                results[i].classes = std::move(fileClasses);
                                results[i].ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
                            }
                            clang_disposeIndex(threadIndex);
                        });
                    }
                    for (auto &th: threads) th.join();

                    for (auto &r: results)
                    {
                        auto orgPath = OrgJsonPath(orgDir, r.headerNorm);
                        WriteOrgJson(orgPath, targetName, r.classes);
                        std::cerr << "[attrfl][parse] " << r.ms << "ms, " << r.classes.size()
                                << " classes: " << r.headerPath << "\n";
                    }
                }
                else
                {
                    CXIndex index = clang_createIndex(0, 0);

                    for (auto &h: toReparse)
                    {
                        auto t0 = std::chrono::steady_clock::now();

                        auto fileClasses = ParseSingleHeader(index, h, normFlags, includeDirsNorm);

                        std::string hNorm = CollectClasses::NormalizePathForCompare(h);
                        auto orgPath = OrgJsonPath(orgDir, hNorm);
                        WriteOrgJson(orgPath, targetName, fileClasses);

                        auto t1 = std::chrono::steady_clock::now();
                        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
                        std::cerr << "[attrfl][parse] " << ms << "ms, " << fileClasses.size()
                                << " classes: " << h << "\n";
                    }

                    clang_disposeIndex(index);
                }
            }

            auto allClasses = LoadAllOrgJsons(orgDir);

            if (!depsArg.empty())
            {
                auto depsClasses = LoadDepsClasses(depsArg);
                for (auto &cls: depsClasses)
                    allClasses.push_back(std::move(cls));
            }

            std::sort(allClasses.begin(), allClasses.end(), [](const ClassInfo &a, const ClassInfo &b)
            {
                if (a.qualifiedName != b.qualifiedName) return a.qualifiedName < b.qualifiedName;
                return a.name < b.name;
            });

            CollectClasses::PropagateClassAttributes(allClasses);
            CollectClasses::CheckMultiInheritConflicts(allClasses);
            CollectClasses::ApplyFieldPriorities(allClasses);
            CollectClasses::ExpandClassAttributesToFields(allClasses);
            CollectClasses::ApplyFieldPriorities(allClasses);
            CollectClasses::StripNoAttributes(allClasses);
            auto processedClasses = CollectClasses::FilterProcessedClasses(allClasses);

            {
                std::ostringstream oss;
                rapidjson::OStreamWrapper osw(oss);
                rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
                TargetClassDetail detail;
                detail.target = targetName;
                detail.generatedAt = GetTimestamp();
                detail.classes = processedClasses;
                detail.Serialize(writer);

                bool written = WriteIfChanged(outJsonPath, oss.str());
                std::cerr << "[attrfl][output] json " << (written ? "written" : "unchanged, skipped")
                        << ": " << outJsonPath << "\n";
            }

            {
                TargetClassDetail detail;
                detail.target = targetName;
                detail.generatedAt = GetTimestamp();
                detail.classes = processedClasses;

                int regCount = 0;
                std::string cppContent = ClassesRegister::GenerateRegisterCodeString(detail, regCount);
                bool written = WriteIfChanged(outCppPath, cppContent);
                std::cerr << "[attrfl][output] cpp " << (written ? "written" : "unchanged, skipped")
                        << ": " << outCppPath << ", registered: " << regCount << "\n";
            }

            WriteStamp(stampPath, flagsHash, includeHash, depsHash, currentMtimes);

            auto finishTime = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count();
            std::cerr << "[attrfl][finish] " << targetName << "Register done, elapsed: " << ms <<
                    " ms\n";
            return 0;
        }
        catch (const std::exception &ex)
        {
            std::cerr << "[attrfl][fatal] unhandled exception: " << ex.what() << "\n";
            return 3;
        }
        catch (...)
        {
            std::cerr << "[attrfl][fatal] unknown exception\n";
            return 4;
        }
    }

    static int GenerateDebug(int argc, char **argv)
    {
        std::string txtPath;

#ifdef _WIN32
        OPENFILENAMEA ofn = {};
        char szFile[MAX_PATH] = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = "attrfl params (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrTitle = "Select attrfl params file";

        if (!GetOpenFileNameA(&ofn))
        {
            std::cerr << "[attrfl][debug] no file selected, cancelled\n";
            return 1;
        }
        txtPath = szFile;
#else
        std::cerr << "[attrfl][debug] enter params file path: ";
        std::getline(std::cin, txtPath);
        if (txtPath.empty())
        {
            std::cerr << "[attrfl][debug] no path entered, cancelled\n";
            return 1;
        }
#endif

        std::ifstream ifs(txtPath);
        if (!ifs)
        {
            std::cerr << "[attrfl][debug] cannot read params file: " << txtPath << "\n";
            return 1;
        }

        std::vector<std::string> argStrings;
        argStrings.push_back("attrfl");
        argStrings.push_back("--generate");

        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty())
                argStrings.push_back(line);
        }

        std::cerr << "[attrfl][debug] loaded " << (argStrings.size() - 2)
                << " params from: " << txtPath << "\n";

        std::vector<char *> fakeArgv;
        fakeArgv.reserve(argStrings.size());
        for (auto &s: argStrings)
            fakeArgv.push_back(const_cast<char *>(s.c_str()));

        if (HasFlag(argc, argv, "--force"))
            fakeArgv.push_back(const_cast<char *>("--force"));

        return Generate(static_cast<int>(fakeArgv.size()), fakeArgv.data());
    }
}
