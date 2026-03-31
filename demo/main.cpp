#include "Reflection.h"
#include <iostream>

#include "Dog.h"
#include "myso.h"
using namespace attrfl;

int main(int argc, char **argv)
{
    Reflect::PrintAllTypes();

    {
        std::cout << "==================UnusedClass======================" << std::endl;

        Reflect::CreateInstance("UnusedClass");
        auto typeInfo = Reflect::GetType("UnusedClass");
        std::cout << "typeInfo: " << typeInfo->GetName() << std::endl;
    }

    {
        std::cout << "==================DOG======================" << std::endl;
        auto dog = Reflect::CreateInstance<Dog>("Dog");
        std::cout << "dog valueA: " << dog->valueA << ", valueB: " << dog->valueB << std::endl;

        auto dogInfo = Reflect::GetType("Dog"); //  dog->GetType();
        auto valueAField = dogInfo->FindField("valueA");
        auto valueBField = dogInfo->FindField("valueB");
        if (!valueBField)
            std::cout << "valueBField is null" << std::endl; // EPNoReflectable

        auto oldValueA = valueAField->GetValue<int>(dog);
        valueAField->SetValue<int>(dog, oldValueA + 1);
        std::cout << "old valueA: " << oldValueA << ", new valueA: " << dog->valueA << std::endl;

        dogInfo->FindField("baseValue")->SetValue<int>(dog, 100);
        std::cout << "baseValue: " << dog->baseValue << std::endl;
    }

    {
        MySO myso;
        myso.Init();
        myso.Save("myso.json");
    }
    {
        MySO myso2;
        myso2.Load("myso.json");
        std::cout << "myso2.baseValue: " << myso2.baseValue << std::endl;
        std::cout << "myso2.value: " << myso2.value << std::endl;
        std::cout << "myso2.dataList: ";
        for (auto &data: myso2.dataList) std::cout << data.baseValue << " ";
        std::cout << std::endl;
        for (auto p: myso2.str2int) std::cout << p.second << " ";
        std::cout << std::endl;
    }
}
