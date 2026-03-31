#pragma once
#include "BaseSO.h"
#include "vector"
#include "unordered_map"
#include "string"

class MyDataSO;

class MySO : public BaseSO
{
    REFLECT_CLASS_DECLARE_C(MySO)
public:
    int value;
    std::vector<MyDataSO> dataList;
    std::unordered_map<std::string, int> str2int;

    void Init();
};

class MyDataSO : public BaseSO
{
    REFLECT_CLASS_DECLARE_C(MyDataSO)
public:
    MyDataSO() = default;

    MyDataSO(int value)
    {
        baseValue = value;
        str = "data" + std::to_string(value);
    }

private:
    std::string str;
};
