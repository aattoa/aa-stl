#pragma once

#include <aa/utility.hpp>

namespace aa::meta {

    template <class...>
    struct List {};
    template <class T>
    concept list = specialization_of<T, List>;

    template <template <class...> class, list>
    struct Apply;
    template <template <class...> class F, class... Ts>
    struct Apply<F, List<Ts...>> : std::type_identity<F<Ts...>> {};

    template <template <class...> class, list>
    struct Map;
    template <template <class...> class F, class... Ts>
    struct Map<F, List<Ts...>> : std::type_identity<List<F<Ts>...>> {};

    template <template <class...> class Trait, class... Ts>
    struct All : std::conjunction<Trait<Ts>...> {};

    template <template <class...> class Trait, class... Ts>
    struct Any : std::disjunction<Trait<Ts>...> {};

    template <class T, template <class...> class... Traits>
    struct Satisfies_all_of : std::conjunction<Traits<T>...> {};

} // namespace aa::meta
