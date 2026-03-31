#pragma once
#include "Generic.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>

namespace fs = std::filesystem;

namespace BaseTypeRegister
{
    static void GenerateBaseTypeRegisterCode(const fs::path &outPath)
    {
        fs::create_directories(outPath.parent_path());
        auto tmpPath = outPath.string() + ".tmp";
        std::ofstream ofs(tmpPath, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open file: " + tmpPath);

        ofs << "//auto generate\n";
        ofs << "#include \"Reflection.h\"\n";
        ofs << "#include <string>\n";
        ofs << "#include <cstdint>\n";
        ofs << "using namespace attrfl;\n";
        ofs << "\n";
        ofs << "namespace{\n";
        ofs << "struct __reflect_register__baseTypes\n";
        ofs << "{\n";
        ofs << "    __reflect_register__baseTypes()\n";
        ofs << "    {\n";

        static constexpr const char *kEntries[] = {
            "int", "unsigned int", "int64_t", "uint64_t",
            "float", "double", "bool", "std::string",
            "unsigned char", "signed char", "std::uint8_t",
        };

        for (const char *typeName: kEntries)
        {
            ofs << "        Reflect::RegisterType<" << typeName
                    << ">(\"" << typeName << "\");\n";
        }

        ofs << "    }\n";
        ofs << "};\n";
        ofs << "static __reflect_register__baseTypes __reflect_baseTypes;\n";
        ofs << "}\n";

        ofs.close();
        fs::rename(tmpPath, outPath);
    }

    static int BaseTypeRegister(int argc, char **argv)
    {
        try
        {
            auto t0 = std::chrono::steady_clock::now();

            auto outPathOpt = GetArgValue(argc, argv, "--out");
            if (!outPathOpt)
            {
                std::cerr << "[attrfl][fatal] missing required arg --out\n";
                return 1;
            }

            std::cerr << "[attrfl][startup] BaseTypeRegister starting" << std::endl;

            GenerateBaseTypeRegisterCode(*outPathOpt);

            auto t1 =  std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            std::cerr << "[attrfl][finish] BaseTypeRegister done, elapsed: " << ms << " ms" << std::endl;
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
}
