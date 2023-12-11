#pragma once

#include <aa/utility.hpp>

#include <concepts>
#include <exception>
#include <functional>
#include <type_traits>

namespace aa {

    struct Bad_maybe_access : std::exception {
        [[nodiscard]] auto what() const noexcept -> char const* override
        {
            return "aa::Bad_maybe_access";
        }
    };

    template <class Config>
    concept maybe_config = requires(bool const has_value) {
        // clang-format off
        { Config::validate_value(has_value) } -> std::same_as<void>;
        { Config::validate_deref(has_value) } -> std::same_as<void>;
        // clang-format on
    };

    // clang-format off

    template <bool check_value, bool check_deref>
    struct Basic_maybe_config final {
        static constexpr auto validate_value(bool const has_value) noexcept(!check_value) -> void
        {
            if constexpr (check_value) if (!has_value) throw Bad_maybe_access {};
        }
        static constexpr auto validate_deref(bool const has_value) noexcept(!check_deref) -> void
        {
            if constexpr (check_deref) if (!has_value) throw Bad_maybe_access {};
        }
    };

    // clang-format on

    using Maybe_config_checked         = Basic_maybe_config<true, true>;
    using Maybe_config_unchecked_deref = Basic_maybe_config<true, false>;
    using Maybe_config_unchecked       = Basic_maybe_config<false, false>;

    struct Nothing final : detail::Internal_tag_type_base {
        explicit consteval Nothing(detail::Internal_construct_tag) {}
    };

    inline constexpr Nothing nothing { detail::Internal_construct_tag {} };

    template <sane T, maybe_config Config = Maybe_config_checked>
    class [[nodiscard]] Maybe final {
        union {
            T m_value;
        };

        bool m_has_value {};

        static constexpr bool nothrow_value = noexcept(Config::validate_value(bool {}));
        static constexpr bool nothrow_deref = noexcept(Config::validate_deref(bool {}));
    public:
        constexpr Maybe(Nothing = nothing) noexcept {} // NOLINT: implicit

        template <class... Args>
            requires std::is_constructible_v<T, Args&&...>
        explicit constexpr Maybe(In_place, Args&&... args) noexcept(
            std::is_nothrow_constructible_v<T, Args&&...>)
            : m_value(std::forward<Args>(args)...)
            , m_has_value(true)
        {}

        template <class Arg = T>
            requires(!tag_type<std::remove_cvref_t<Arg>>)
                 && (!std::is_same_v<Maybe, std::remove_cvref_t<Arg>>)
                 && std::is_constructible_v<T, Arg&&>
        explicit(!std::is_convertible_v<Arg&&, T>) constexpr Maybe(Arg&& arg) // NOLINT
            : Maybe(in_place, std::forward<Arg>(arg))
        {}

        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access, bugprone-macro-parentheses)

        Maybe(Maybe const&)
            requires(!std::is_copy_constructible_v<T>)
        = delete;
        Maybe(Maybe const&)
            requires std::is_trivially_copy_constructible_v<T>
        = default;

        constexpr Maybe(Maybe const& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires std::is_copy_constructible_v<T> && (!std::is_trivially_copy_constructible_v<T>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), other.m_value);
            }
        }

        Maybe(Maybe&&) noexcept
            requires(!std::is_move_constructible_v<T>)
        = delete;
        Maybe(Maybe&&) noexcept
            requires std::is_trivially_move_constructible_v<T>
        = default;

        constexpr Maybe(Maybe&& other) noexcept
            requires std::is_move_constructible_v<T> && (!std::is_trivially_move_constructible_v<T>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), std::move(other.m_value));
            }
        }

        auto operator=(Maybe const&) -> Maybe&
            requires(!std::is_copy_assignable_v<T>)
        = delete;
        auto operator=(Maybe const&) -> Maybe&
            requires std::is_trivially_copy_assignable_v<T>
        = default;

        constexpr auto operator=(Maybe const& other) noexcept(nothrow_copyable<T>) -> Maybe&
            requires std::is_copy_constructible_v<T> && (!std::is_trivially_copy_assignable_v<T>)
        {
            if (this != &other) {
                if (!other.m_has_value) {
                    reset();
                }
                else {
                    if constexpr (std::is_copy_assignable_v<T>) {
                        if (m_has_value) {
                            m_value = other.m_value;
                            return *this;
                        }
                    }
                    emplace(other.m_value);
                }
            }
            return *this;
        }

        auto operator=(Maybe&&) -> Maybe&
            requires(!std::is_move_assignable_v<T>)
        = delete;
        auto operator=(Maybe&&) noexcept -> Maybe&
            requires std::is_trivially_move_assignable_v<T>
        = default;

        constexpr auto operator=(Maybe&& other) noexcept -> Maybe&
            requires std::is_move_constructible_v<T> && (!std::is_trivially_move_assignable_v<T>)
        {
            if (this != &other) {
                if (!other.m_has_value) {
                    reset();
                }
                else {
                    if constexpr (std::is_move_assignable_v<T>) {
                        if (m_has_value) {
                            m_value = std::move(other.m_value);
                            return *this;
                        }
                    }
                    emplace(std::move(other.m_value));
                }
            }
            return *this;
        }

        ~Maybe()
            requires(std::is_trivially_destructible_v<T>)
        = default;

        constexpr ~Maybe()
            requires(!std::is_trivially_destructible_v<T>)
        {
            reset();
        }

        constexpr auto reset() noexcept -> void
        {
            if (m_has_value) {
                std::destroy_at(std::addressof(m_value));
                m_has_value = false;
            }
        }

        [[nodiscard]] constexpr auto has_value() const noexcept -> bool
        {
            return m_has_value;
        }

        [[nodiscard]] constexpr auto is_empty() const noexcept -> bool
        {
            return !m_has_value;
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return m_has_value;
        }

        template <class... Args>
        constexpr auto emplace(Args&&... args) noexcept(
            std::is_nothrow_constructible_v<T, Args&&...>) -> T&
        {
            reset();
            T* const ptr = std::construct_at(std::addressof(m_value), std::forward<Args>(args)...);
            m_has_value  = true;
            return *ptr;
        }

        template <class Self>
        [[nodiscard]] constexpr auto value(this Self&& self) noexcept(nothrow_value)
            -> Qualified_like<Self, T>
        {
            Config::validate_value(self.m_has_value);
            return std::forward_like<Self>(self.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto value_unchecked(this Self&& self) noexcept
            -> Qualified_like<Self, T>
        {
            return std::forward_like<Self>(self.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto operator*(this Self&& self) noexcept(nothrow_deref)
            -> Qualified_like<Self, T>
        {
            Config::validate_deref(self.m_has_value);
            return std::forward_like<Self>(self.m_value);
        }

        [[nodiscard]] constexpr auto operator->(this auto&& self) noexcept(nothrow_deref)
        {
            Config::validate_deref(self.m_has_value);
            return std::addressof(self.m_value);
        }

        template <class Self, std::invocable<Qualified_like<Self, T>> Function>
        [[nodiscard]] constexpr auto map(this Self&& self, Function&& function) noexcept(
            std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>)
            -> Maybe<std::invoke_result_t<Function&&, Qualified_like<Self, T>>>
        {
            if (!self.m_has_value) {
                return {};
            }
            return Maybe<std::invoke_result_t<Function, Qualified_like<Self, T>>>(
                in_place,
                std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_value)));
        }

        template <class Self, std::invocable<Qualified_like<Self, T>> Function>
        constexpr auto map(this Self&& self, Function&& function) noexcept(
            std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>) -> void
            requires std::is_void_v<std::invoke_result_t<Function&&, Qualified_like<Self, T>>>
        {
            if (self.m_has_value) {
                std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_value));
            }
        }

        // NOLINTEND(cppcoreguidelines-pro-type-union-access, bugprone-macro-parentheses)
    };

} // namespace aa

namespace aa::inline basics {
    using aa::Maybe;
    using aa::nothing;
} // namespace aa::inline basics
