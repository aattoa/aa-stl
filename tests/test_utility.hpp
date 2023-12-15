#pragma once

#include <aa/maybe.hpp>

#define STATIC_TEST(name, ...) static_assert(([] consteval -> bool __VA_ARGS__)(), name);

namespace aa::tests {

    struct Nontrivial {
        int integer {};

        Nontrivial() = default;

        explicit constexpr Nontrivial(int const value) : integer(value) {}

        constexpr Nontrivial(Nontrivial const& other) noexcept // NOLINT: equals default
            : integer(other.integer) {};

        constexpr Nontrivial(Nontrivial&& other) noexcept : integer(std::exchange(other.integer, 0))
        {}

        constexpr auto operator=(Nontrivial const& other) -> Nontrivial&
        {
            if (this != &other) {
                integer = other.integer;
            }
            return *this;
        };

        constexpr auto operator=(Nontrivial&& other) noexcept -> Nontrivial&
        {
            if (this != &other) {
                integer = std::exchange(other.integer, 0);
            }
            return *this;
        }

        constexpr ~Nontrivial() {} // NOLINT: equals default

        [[nodiscard]] constexpr auto operator==(int const value) const noexcept -> bool
        {
            return integer == value;
        }
    };

    struct Nontrivial_with_sentinel : Nontrivial {
        static constexpr int sentinel = 1000;
    };

    static_assert(
        !std::is_trivially_default_constructible_v<Nontrivial>
        && !std::is_trivially_copy_assignable_v<Nontrivial>
        && !std::is_trivially_copy_constructible_v<Nontrivial>
        && !std::is_trivially_move_assignable_v<Nontrivial>
        && !std::is_trivially_move_constructible_v<Nontrivial>
        && !std::is_trivially_destructible_v<Nontrivial>);

    static_assert(aa::sane<Nontrivial> && aa::sane<Nontrivial_with_sentinel>);

} // namespace aa::tests

template <>
struct aa::Maybe_config_default_for<aa::tests::Nontrivial_with_sentinel> final
    : Maybe_config_checked {
    static constexpr auto sentinel_value() noexcept -> aa::tests::Nontrivial_with_sentinel
    {
        return aa::tests::Nontrivial_with_sentinel { aa::tests::Nontrivial {
            aa::tests::Nontrivial_with_sentinel::sentinel } };
    }
    static constexpr auto is_sentinel_value(aa::tests::Nontrivial_with_sentinel const& x) noexcept
        -> bool
    {
        return x.integer == aa::tests::Nontrivial_with_sentinel::sentinel;
    }
};
