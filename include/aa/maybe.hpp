#pragma once

#include <aa/utility.hpp>

namespace aa::dtl {
    template <sane T, sentinel_config<T> Config>
    struct Maybe_core;

    template <sane T, sentinel_config<T> Config>
        requires std::is_same_v<T, decltype(Config::sentinel_value())>
    struct Maybe_core<T, Config> final {
        T m_value = Config::sentinel_value();

        Maybe_core() = default;

        template <class... Args>
        explicit constexpr Maybe_core(In_place, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : m_value(std::forward<Args>(args)...)
        {}

        [[nodiscard]] constexpr auto has_value() const
            noexcept(noexcept(Config::is_sentinel_value(m_value))) -> bool
        {
            return !Config::is_sentinel_value(m_value);
        }

        template <class... Args>
        constexpr auto emplace(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> void
        {
            move_assign(m_value, T(std::forward<Args>(args)...));
        }

        constexpr auto reset() noexcept -> void
            requires(std::is_move_constructible_v<T> || std::is_move_assignable_v<T>)
        {
            move_assign(m_value, Config::sentinel_value());
        }
    };

    template <sane T, sentinel_config<T> Config>
        requires std::is_void_v<decltype(Config::sentinel_value())>
    struct Maybe_core<T, Config> final {
        union {
            T m_value;
        };
        bool m_has_value = false;

        constexpr Maybe_core() noexcept {}

        template <class... Args>
        explicit constexpr Maybe_core(In_place, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : m_value(std::forward<Args>(args)...)
            , m_has_value(true)
        {}

        [[nodiscard]] constexpr auto has_value() const noexcept -> bool
        {
            return m_has_value;
        }

        ~Maybe_core()
            requires(std::is_trivially_destructible_v<T>)
        = default;

        constexpr ~Maybe_core()
            requires(!std::is_trivially_destructible_v<T>)
        {
            reset();
        }

        template <class... Args>
        constexpr auto emplace(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> void
        {
            reset();
            std::construct_at(
                std::addressof(m_value), std::forward<Args>(args)...); // NOLINT: union access
            m_has_value = true;
        }

        constexpr auto reset() noexcept -> void
        {
            if (m_has_value) {
                std::destroy_at(std::addressof(m_value)); // NOLINT: union access
                m_has_value = false;
            }
        }

        Maybe_core(Maybe_core const&)
            requires(!std::is_copy_constructible_v<T>)
        = delete;
        Maybe_core(Maybe_core const&)
            requires std::is_trivially_copy_constructible_v<T>
        = default;

        constexpr Maybe_core(Maybe_core const& other)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires std::is_copy_constructible_v<T> && (!std::is_trivially_copy_constructible_v<T>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), other.m_value); // NOLINT: union access
            }
        }

        Maybe_core(Maybe_core&&) noexcept
            requires(!std::is_move_constructible_v<T>)
        = delete;
        Maybe_core(Maybe_core&&) noexcept
            requires std::is_trivially_move_constructible_v<T>
        = default;

        constexpr Maybe_core(Maybe_core&& other) noexcept
            requires std::is_move_constructible_v<T> && (!std::is_trivially_move_constructible_v<T>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value) {
                std::construct_at(
                    std::addressof(m_value), std::move(other.m_value)); // NOLINT: union access
            }
        }

        auto operator=(Maybe_core const&) -> Maybe_core&
            requires(!std::is_copy_assignable_v<T>)
        = delete;
        auto operator=(Maybe_core const&) -> Maybe_core&
            requires std::is_trivially_copy_assignable_v<T>
        = default;

        constexpr auto operator=(Maybe_core const& other)
            noexcept(nothrow_copyable<T>) -> Maybe_core&
            requires std::is_copy_constructible_v<T> && (!std::is_trivially_copy_assignable_v<T>)
        {
            if (this != &other) {
                if (!other.m_has_value) {
                    reset();
                }
                else {
                    if constexpr (std::is_copy_assignable_v<T>) {
                        if (m_has_value) {
                            m_value = other.m_value; // NOLINT: union access
                            return *this;
                        }
                    }
                    emplace(other.m_value);
                }
            }
            return *this;
        }

        auto operator=(Maybe_core&&) -> Maybe_core&
            requires(!std::is_move_assignable_v<T>)
        = delete;
        auto operator=(Maybe_core&&) noexcept -> Maybe_core&
            requires std::is_trivially_move_assignable_v<T>
        = default;

        constexpr auto operator=(Maybe_core&& other) noexcept -> Maybe_core&
            requires std::is_move_constructible_v<T> && (!std::is_trivially_move_assignable_v<T>)
        {
            if (this != &other) {
                if (!other.m_has_value) {
                    reset();
                }
                else {
                    if constexpr (std::is_move_assignable_v<T>) {
                        if (m_has_value) {
                            m_value = std::move(other.m_value); // NOLINT: union access
                            return *this;
                        }
                    }
                    emplace(std::move(other.m_value));
                }
            }
            return *this;
        }
    };
} // namespace aa::dtl

namespace aa {

    struct Nothing final : detail::Internal_tag_type_base {
        explicit consteval Nothing(detail::Internal_construct_tag) {}
    };
    inline constexpr Nothing nothing { detail::Internal_construct_tag {} };

    template <
        sane               T,
        access_config      Unwrap_config   = Access_config_checked,
        access_config      Deref_config    = Access_config_checked,
        sentinel_config<T> Sentinel_config = Sentinel_config_default_for<T>>
    class [[nodiscard]] Maybe final {
        dtl::Maybe_core<T, Sentinel_config> m_core;

        static constexpr bool nothrow_unwrap = noexcept(Unwrap_config::validate_access(bool {}));
        static constexpr bool nothrow_deref  = noexcept(Deref_config::validate_access(bool {}));
    public:
        constexpr Maybe(Nothing = nothing) noexcept {} // NOLINT: implicit

        template <class... Args>
        explicit constexpr Maybe(In_place, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            requires std::is_constructible_v<T, Args&&...>
            : m_core(in_place, std::forward<Args>(args)...)
        {}

        template <class Arg = T>
            requires(!tag_type<std::remove_cvref_t<Arg>>)
                 && (!std::is_same_v<Maybe, std::remove_cvref_t<Arg>>)
                 && std::is_constructible_v<T, Arg&&>
        explicit(!std::is_convertible_v<Arg&&, T>) constexpr Maybe(Arg&& arg) // NOLINT
            : Maybe(in_place, std::forward<Arg>(arg))
        {}

        [[nodiscard]] constexpr auto has_value() const noexcept -> bool
        {
            return m_core.has_value();
        }

        [[nodiscard]] constexpr auto is_empty() const noexcept -> bool
        {
            return !has_value();
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return has_value();
        }

        constexpr auto reset() noexcept(noexcept(m_core.reset())) -> void
        {
            m_core.reset();
        }

        template <class... Args>
        constexpr auto emplace(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> T&
        {
            m_core.emplace(std::forward<Args>(args)...);
            return m_core.m_value;
        }

        template <class Self>
        [[nodiscard]] constexpr auto unwrap(this Self&& self)
            noexcept(nothrow_unwrap) -> Qualified_like<Self, T>
        {
            Unwrap_config::validate_access(self.has_value());
            return std::forward_like<Self>(self.m_core.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto unwrap_unchecked(this Self&& self) noexcept
            -> Qualified_like<Self, T>
        {
            return std::forward_like<Self>(self.m_core.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto operator*(this Self&& self)
            noexcept(nothrow_deref) -> Qualified_like<Self, T>
        {
            Deref_config::validate_access(self.has_value());
            return std::forward_like<Self>(self.m_core.m_value);
        }

        [[nodiscard]] constexpr auto operator->(this auto&& self)
            noexcept(nothrow_deref) -> decltype(std::addressof(self.m_core.m_value))
        {
            Deref_config::validate_access(self.has_value());
            return std::addressof(self.m_core.m_value);
        }

        template <class Self, std::invocable<Qualified_like<Self, T>> Function>
        [[nodiscard]] constexpr auto map(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>)
                -> Maybe<
                    std::invoke_result_t<Function&&, Qualified_like<Self, T>>,
                    Unwrap_config,
                    Deref_config>
        {
            if (!self.has_value()) {
                return nothing;
            }
            return Maybe<
                std::invoke_result_t<Function, Qualified_like<Self, T>>,
                Unwrap_config,
                Deref_config>(
                in_place,
                std::invoke(
                    std::forward<Function>(function),
                    std::forward_like<Self>(self.m_core.m_value)));
        }

        template <class Self, std::invocable<Qualified_like<Self, T>> Function>
        constexpr auto map(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>) -> void
            requires std::is_void_v<std::invoke_result_t<Function&&, Qualified_like<Self, T>>>
        {
            if (self.has_value()) {
                std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_core.m_value));
            }
        }

        [[nodiscard]] constexpr auto ref() & noexcept -> Maybe<Ref<T>, Unwrap_config, Deref_config>
        {
            if (has_value()) {
                return Ref { m_core.m_value };
            }
            return nothing;
        }

        [[nodiscard]] constexpr auto ref() const& noexcept
            -> Maybe<Ref<T const>, Unwrap_config, Deref_config>
        {
            if (has_value()) {
                return Ref { m_core.m_value };
            }
            return nothing;
        }

        auto ref() &&      = delete;
        auto ref() const&& = delete;
    };

} // namespace aa

namespace aa::inline basics {
    using aa::Maybe;
    using aa::nothing;
} // namespace aa::inline basics
