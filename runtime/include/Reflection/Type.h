#pragma once
#include "Reflection/TypeInfo.h"
#include "Reflection/ContainerTraits.h"
#include <typeinfo>
#include <type_traits>
#include <mutex>

namespace attrfl
{
    template<typename T>
    class Type;

    template<typename T, bool IsClass = std::is_class_v<T> >
    struct TypeClassMethods
    {
    };

    template<typename T>
    struct TypeClassMethods<T, true>
    {
        template<typename... Parents>
        static void SetParent()
        {
            static_assert((std::is_base_of_v<Parents, T> && ...),
                          "Parent must be a base class of T");
            (void) std::initializer_list<int>{(Type<T>::Get()->AddParent(Type<Parents>::Get()), 0)...};
        }

        template<typename FieldType>
        static void RegisterField(const std::string &fieldName,
                                  const std::string &access,
                                  FieldType T::*memberPtr,
                                  std::vector<AttributeInfo> attrs = {})
        {
            FieldInfo field;
            field.name = fieldName;
            field.attributes = std::move(attrs);

            field.type = Type<FieldType>::Get();
            if (access == "public")
                field.access = FieldInfo::Access::Public;
            else if (access == "protected")
                field.access = FieldInfo::Access::Protected;
            else
                field.access = FieldInfo::Access::Private;

            alignas(T) unsigned char offsetBuf[sizeof(T)];
            T *dummyObj = reinterpret_cast<T *>(offsetBuf);
            field.offset = reinterpret_cast<size_t>(&(dummyObj->*memberPtr))
                           - reinterpret_cast<size_t>(dummyObj);
            field.fieldSize = sizeof(FieldType);

            field.RebuildAttributeSet();

            if (field.type)
            {
                field.containerKind = field.type->containerKind;
            }

            Type<T>::Get()->AddField(std::move(field));
        }
    };

    template<typename T>
    class Type : public TypeClassMethods<T>
    {
    private:
        static TypeInfo *typeInfo;

        static constexpr bool kCanCreate =
                !std::is_abstract_v<T> && std::is_default_constructible_v<T>;

        static void *CreateInstance()
        {
            if constexpr (kCanCreate)
                return new T();
            else
                return nullptr;
        }

        static TypeInfo *CreateTypeInfo()
        {
            std::string typeName = typeid(T).name();

#ifdef _MSC_VER
            if (typeName.rfind("class ", 0) == 0)
                typeName = typeName.substr(6);
            else if (typeName.rfind("struct ", 0) == 0)
                typeName = typeName.substr(7);
            else if (typeName.rfind("enum ", 0) == 0)
                typeName = typeName.substr(5);
#endif

            auto *info = new TypeInfo(
                typeName,
                std::type_index(typeid(T)),
                sizeof(T),
                kCanCreate ? &CreateInstance : nullptr
            );

            if constexpr (ContainerTraits<T>::Kind == ContainerKind::List)
            {
                using V = typename ContainerTraits<T>::ValueType;
                info->containerKind = ContainerKind::List;
                info->valueType = Type<V>::Get();
                info->listGetSize = [](const void *ptr) -> size_t
                {
                    return static_cast<const std::vector<V> *>(ptr)->size();
                };
                info->listGetElement = [](const void *ptr, size_t idx) -> const void *
                {
                    return &(*static_cast<const std::vector<V> *>(ptr))[idx];
                };
                info->listClear = [](void *ptr)
                {
                    static_cast<std::vector<V> *>(ptr)->clear();
                };
                info->listAddDefault = [](void *ptr) -> void *
                {
                    auto *vec = static_cast<std::vector<V> *>(ptr);
                    vec->emplace_back();
                    return &vec->back();
                };
            }
            else if constexpr (ContainerTraits<T>::Kind == ContainerKind::Dict)
            {
                using K = typename ContainerTraits<T>::KeyType;
                using V = typename ContainerTraits<T>::ValueType;
                info->containerKind = ContainerKind::Dict;
                info->keyType = Type<K>::Get();
                info->valueType = Type<V>::Get();
                info->dictForEach = [](const void *ptr,
                                       std::function<void(const std::string &, const void *)> cb)
                {
                    const auto *map = static_cast<const std::unordered_map<K, V> *>(ptr);
                    for (const auto &[k, v]: *map)
                    {
                        std::string keyStr;
                        if constexpr (std::is_same_v<K, std::string>)
                            keyStr = k;
                        else
                            keyStr = std::to_string(k);
                        cb(keyStr, &v);
                    }
                };
                info->dictClear = [](void *ptr)
                {
                    static_cast<std::unordered_map<K, V> *>(ptr)->clear();
                };
                info->dictInsert = [](void *ptr, std::string_view keyStr) -> void *
                {
                    auto *map = static_cast<std::unordered_map<K, V> *>(ptr);
                    K key;
                    if constexpr (std::is_same_v<K, std::string>)
                        key = std::string(keyStr);
                    else if constexpr (std::is_integral_v<K>)
                        key = static_cast<K>(std::stoll(std::string(keyStr)));
                    else
                    {
                        static_assert(std::is_same_v<K, std::string> || std::is_integral_v<K>,
                                      "Unsupported dict key type");
                        key = K{};
                    }
                    return &(*map)[key];
                };
            }

            return info;
        }

    public:
        static TypeInfo *Get()
        {
            static std::once_flag flag;
            std::call_once(flag, []()
            {
                typeInfo = CreateTypeInfo();
            });
            return typeInfo;
        }

        static const std::string &GetName()
        {
            return Get()->GetName();
        }

        static size_t GetSize()
        {
            return Get()->GetSize();
        }

        template<typename U>
        static constexpr bool IsA()
        {
            return std::is_base_of_v<U, T> || std::is_same_v<U, T>;
        }

        static bool IsA(const TypeInfo *other)
        {
            return Get()->IsA(other);
        }

        static void SetAttributes(std::vector<AttributeInfo> attrs)
        {
            for (auto &a: attrs) Get()->AddAttribute(std::move(a));
        }
    };

    template<typename T>
    TypeInfo *Type<T>::typeInfo = nullptr;
}
