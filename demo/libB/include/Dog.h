#pragma once
#include "Animal.h"
class Dog :public Animal
{
    REFLECT_CLASS_DECLARE_C(Dog)
public:
    Dog();
    int valueA;
    EFIELD(EPNoReflectable)
    int valueB;
};