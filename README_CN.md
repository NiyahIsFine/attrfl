[English](README.md)

# attrfl

基于 C++ Attribute 的编译期反射与序列化代码生成框架。通过 libclang 解析标注，构建时自动生成反射注册代码。

## 特点

- `ECLASS` / `EFIELD` 宏标注，零手写注册
- 自动识别继承链，支持字段级反射/序列化控制
- CMake 集成，几行配置即可接入

## 用法

### 1. CMake 配置
```cmake
include(path/to/runtime/cmake/Attrfl.cmake)

set(ATTRFL_EXE "path/to/attrfl.exe")
set(ATTRFL_MULTITHREAD ON)
set(ATTRFL_NOSTDINC ON)

attrfl_gen(libA demo_exe)
attrfl_gen(libB demo_exe)
attrfl_finalize()
```
### 2. 使用反射
```cpp
class ECLASS(EPClassReflectable(true, true)) Animal
{
    REFLECT_CLASS_DECLARE(Animal)
public:
    int baseValue = 5;
};
class Dog :public Animal
{
    REFLECT_CLASS_DECLARE_C(Animal)
public:
    in valueA = 5;
}
// 运行时
auto dog = Reflect::CreateInstance<Animal>("Dog"); //Reflect::CreateInstance<Dog>("Dog");
auto field = Reflect::GetType("Dog")->FindField("valueA");
field->SetValue<int>(dog, 42);
```

## 依赖

- C++17 / CMake ≥ 3.20 / libclang

## 第三方

- [LLVM/Clang](https://github.com/llvm/llvm-project) — Apache 2.0 with LLVM Exception
- [RapidJSON](https://github.com/Tencent/rapidjson) — MIT © 2015 THL A29 Limited, Milo Yip

## 许可证

[MIT](LICENSE)