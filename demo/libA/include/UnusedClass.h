#pragma once

#include "Reflection.h"
#include <iostream>
class ECLASS(EPClassReflectable(false, true)) UnusedClass
{
    REFLECT_CLASS_DECLARE(UnusedClass)
    public:
    UnusedClass() : value(10)
    {
        std::cout << "UnusedClass::UnusedClass()" << std::endl;
    } ;
    int value;
};