#pragma once
#include "Reflection.h"
class ECLASS(EPClassReflectable(true, true)) Animal
{
    REFLECT_CLASS_DECLARE(Animal)
    public:
    int baseValue = 5;
};