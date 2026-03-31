#pragma once

#include <vector>
#include <unordered_map>

namespace attrfl
{
    enum class ContainerKind
    {
        None, //
        List, // std::vector
        Dict, // std::unordered_map
    };

    template<typename T>
    struct ContainerTraits
    {
        static constexpr ContainerKind Kind = ContainerKind::None;
    };

    template<typename V>
    struct ContainerTraits<std::vector<V> >
    {
        static constexpr ContainerKind Kind = ContainerKind::List;
        using ValueType = V;
    };

    template<typename K, typename V>
    struct ContainerTraits<std::unordered_map<K, V> >
    {
        static constexpr ContainerKind Kind = ContainerKind::Dict;
        using KeyType = K;
        using ValueType = V;
    };
}
