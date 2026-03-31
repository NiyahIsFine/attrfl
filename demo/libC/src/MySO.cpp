#include "MySO.h"

void MySO::Init()
{
    baseValue = 10;
    baseValue2 = 11; //EPNoSerializable
    baseValue3 = 12; //EPNoReflectable (EPNoSerializable)

    value = 20;
    dataList.emplace_back(1);
    dataList.emplace_back(2);

    str2int["_1"] = 1;
    str2int["_2"] = 2;
}
