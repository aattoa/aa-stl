#pragma once

#include <type_traits>
#include <utility>

namespace aa::detail {
    struct Internal_tag_type_base {};

    struct Internal_construct_tag final {
        explicit consteval Internal_construct_tag() = default;
    };
} // namespace aa::detail

namespace aa {

    struct In_place final : detail::Internal_tag_type_base {
        explicit consteval In_place(detail::Internal_construct_tag) {}
    };

    template <class>
    struct In_place_type final : detail::Internal_tag_type_base {
        explicit consteval In_place_type(detail::Internal_construct_tag) {}
    };

    inline constexpr In_place in_place { detail::Internal_construct_tag {} };

    template <class T>
    constexpr In_place_type<T> in_place_type { detail::Internal_construct_tag {} };

    template <class T>
    concept tag_type = std::is_base_of_v<detail::Internal_tag_type_base, T>;

    // Instances of sane types can be reasonably stored in containers, with no surprising behavior.
    template <class T>
    concept sane = requires {
        requires !tag_type<T>;
        requires std::is_nothrow_destructible_v<T>;
        requires std::is_nothrow_move_constructible_v<T> || !std::is_move_constructible_v<T>;
        requires std::is_nothrow_move_assignable_v<T> || !std::is_move_assignable_v<T>;
        requires std::is_nothrow_swappable_v<T> || !std::is_swappable_v<T>;
        requires std::is_trivially_destructible_v<T> || !std::is_trivially_copyable_v<T>;
    };

    template <class T>
    concept nothrow_copyable
        = std::is_nothrow_copy_constructible_v<T>
       && (!std::is_copy_assignable_v<T> || std::is_nothrow_copy_assignable_v<T>);

    template <class T>
    concept nothrow_movable
        = std::is_nothrow_move_constructible_v<T>
       && (!std::is_move_assignable_v<T> || std::is_nothrow_move_assignable_v<T>);

} // namespace aa
