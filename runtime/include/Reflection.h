#pragma once

#include "Reflection/Type.h"
#include "Reflection/TypeInfo.h"
#include <vector>
#include <string>
#include <iostream>

namespace attrfl
{
    class Reflect
    {
    private:
        static bool RegisterTypeInternal(TypeInfo *type, const std::string &typeName);

        static const std::vector<TypeInfo *> &GetAllTypesInternal();

    public:
        template<typename T>
        static void RegisterType(const std::string &name)
        {
            TypeInfo *type = Type<T>::Get();

            std::string typeName = name.empty() ? type->GetName() : name;
            RegisterTypeInternal(type, typeName);
        }

        template<typename T, typename... Parents, typename = std::enable_if_t<(sizeof...(Parents) > 0)> >
        static void RegisterType(const std::string &name = "")
        {
            RegisterType<T>(name);

            Type<T>::template SetParent<Parents...>();
        }
        
        static TypeInfo *GetType(const char *name)
        {
            return GetType(std::string(name));
        }

        static TypeInfo *GetType(const std::string &name);

        static TypeInfo *GetType(std::type_index index);


        template<typename T>
        static TypeInfo *GetType()
        {
            return GetType(std::type_index(typeid(T)));
        }

        template<typename T>
        static TypeInfo *GetType(const T *obj)
        {
            if (!obj) return nullptr;
            return GetType(std::type_index(typeid(*obj)));
        }

        template<typename T = void>
        static T *CreateInstance(const std::string &name)
        {
            return CreateInstance<T>(GetType(name));
        }

        template<typename T = void>
        static T *CreateInstance(const TypeInfo *type)
        {
            if (!type)
            {
                std::cout << "[error]" << "Cannot create instance: TypeInfo is null" << std::endl;
                return nullptr;
            }

            void *instance = type->CreateInstance();
            if (!instance)
            {
                std::cout << "[error]" << "Cannot create instance: Constructor failed for '{" << type->GetName() << "}'"
                        << std::endl;
                return nullptr;
            }

            if constexpr (std::is_same_v<T, void>)
            {
                return instance;
            }
            else
            {
                return static_cast<T *>(instance);
            }
        }

        static std::vector<std::string> GetAllTypeNames();


        static void PrintAllTypes();

        static const std::vector<TypeInfo *> &GetAllTypes()
        {
            return GetAllTypesInternal();
        }


        template<typename T>
        static std::vector<TypeInfo *> GetDerivedTypes()
        {
            std::vector<TypeInfo *> out;
            TypeInfo *base = GetType<T>();
            for (TypeInfo *t: GetAllTypesInternal())
            {
                if (t != base && t->IsDerivedFrom(base)) out.push_back(t);
            }
            return out;
        }

        template<typename T>
        static std::vector<std::string> GetDerivedTypeNames()
        {
            std::vector<std::string> names;
            TypeInfo *base = GetType<T>();
            if (!base) return names;

            for (TypeInfo *t: GetAllTypesInternal())
            {
                if (!t) continue;
                if (t != base && t->IsDerivedFrom(base))
                    names.push_back(t->GetName());
            }
            return names;
        }
    };
}

#define REFLECT_CLASS_DECLARE(ClassName) \
public: \
static attrfl::TypeInfo* StaticType() { return attrfl::Reflect::GetType<ClassName>(); } \
virtual attrfl::TypeInfo* GetType() const { return StaticType(); } \
virtual const char* GetTypeName() const { return #ClassName; } \
static void __RegisterField(); \
private:

#define REFLECT_CLASS_DECLARE_C(ClassName) \
public: \
static attrfl::TypeInfo* StaticType() { return attrfl::Reflect::GetType<ClassName>(); } \
virtual attrfl::TypeInfo* GetType() const override { return StaticType(); } \
virtual const char* GetTypeName() const override { return #ClassName; } \
static void __RegisterField(); \
private:


#if defined(__has_attribute)
#if __has_attribute(annotate)
#define ATTRFL_ANNOTATE(x) __attribute__((annotate(x)))
#else
#define ATTRFL_ANNOTATE(x)
#endif
#elif defined(__clang__) || defined(__GNUC__)
#define ATTRFL_ANNOTATE(x) __attribute__((annotate(x)))
#else
#define ATTRFL_ANNOTATE(x)
#endif

#define ATTRFL_STRINGIFY_INNER(...) #__VA_ARGS__
#define ATTRFL_STRINGIFY(...) ATTRFL_STRINGIFY_INNER(__VA_ARGS__)

#define EFIELD_IMPL(...)  ATTRFL_ANNOTATE("field:" ATTRFL_STRINGIFY(__VA_ARGS__))
#define ECLASS_IMPL(...)  ATTRFL_ANNOTATE("class:" ATTRFL_STRINGIFY(__VA_ARGS__))

#define EFIELD(...) EFIELD_IMPL(__VA_ARGS__)
#define ECLASS(...) ECLASS_IMPL(__VA_ARGS__)

/*
 * param1: childApply boolean
 * param2: fieldCollect boolean
 */
#define EPClassReflectable classReflectable

#define EPReflectable reflectable

#define EPNoReflectable noReflectable

/*
 * param1: childApply boolean
 */
#define EPClassSerializable classSerializable

#define EPSerializable serializable

#define EPNoSerializable noSerializable

/*
 * param1: inheritSelf boolean
 */
#define EPClassFinalReflectable classFinalReflectable

/*
 * param1: inheritSelf boolean
 */
#define EPClassFinalSerializable classFinalSerializable


#define EP_PARAM_STRING(v) AttributeParam::MakeString(v)

#define EP_PARAM_BOOL(v)   AttributeParam::MakeBool(v)

#define EP_PARAM_INT(v)    AttributeParam::MakeInt(v)

#define EP_PARAM_FLOAT(v)  AttributeParam::MakeFloat(v)
