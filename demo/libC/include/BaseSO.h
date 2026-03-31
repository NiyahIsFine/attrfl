#pragma once
#include "Reflection.h"
#include <filesystem>
class ECLASS(EPClassSerializable(true)) BaseSO
{
    REFLECT_CLASS_DECLARE(BaseSO)
public:
    BaseSO();
    int baseValue = 0;
    EFIELD(EPNoSerializable)
    int baseValue2 = 0;
    EFIELD(EPNoReflectable)
    int baseValue3 = 0;

    void Load(std::filesystem::path filePath);
    void Save(std::filesystem::path filePath);

};