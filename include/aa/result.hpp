#pragma once

#include <aa/meta.hpp>
#include <aa/maybe.hpp>
#include <aa/utility.hpp>

namespace aa {

    template <class T>
    struct [[nodiscard]] Error final {
        T value;
    };

    template <
        sane          T,
        sane          E,
        access_config Unwrap_config = Access_config_checked,
        access_config Deref_config  = Access_config_checked>
    class [[nodiscard]] Result final {
        union {
            T m_value;
            E m_error;
        };
        bool m_has_value {};

        static constexpr bool nothrow_unwrap = noexcept(Unwrap_config::validate_access(bool {}));
        static constexpr bool nothrow_deref  = noexcept(Deref_config::validate_access(bool {}));
    public:
        constexpr Result() noexcept(std::is_nothrow_default_constructible_v<T>)
            requires std::is_default_constructible_v<T>
            : m_value()
            , m_has_value(true)
        {}

        template <class Arg = T>
            requires(!tag_type<std::remove_cvref_t<Arg>>)
                 && (!std::is_same_v<Result, std::remove_cvref_t<Arg>>)
                 && std::is_constructible_v<T, Arg&&>
        explicit(!std::is_convertible_v<Arg&&, T>) constexpr Result(Arg&& arg)
            noexcept(noexcept(Result(in_place, std::forward<Arg>(arg))))
            : Result(in_place, std::forward<Arg>(arg))
        {}

        template <class... Args>
        explicit constexpr Result(In_place, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            requires std::is_constructible_v<T, Args&&...>
            : m_value(std::forward<Args>(args)...)
            , m_has_value(true)
        {}

        template <class Err>
        constexpr Result(Err&& err) noexcept // NOLINT: bugprone forwarding reference
            requires std::is_same_v<Error<E>, std::remove_cvref_t<Err>>
            : m_error(std::forward<Err>(err).value)
        {}

        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)

        Result(Result const&)
            requires(!meta::All<std::is_copy_constructible, T, E>::value)
        = delete;
        Result(Result const&)
            requires meta::All<std::is_trivially_copy_constructible, T, E>::value
        = default;

        constexpr Result(Result const& other)
            noexcept(meta::All<std::is_nothrow_copy_constructible, T, E>::value)
            requires meta::All<std::is_copy_constructible, T, E>::value
            : m_has_value(other.m_has_value)
        {
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), other.m_value);
            }
            else {
                std::construct_at(std::addressof(m_error), other.m_error);
            }
        }

        Result(Result&&)
            requires(!meta::All<std::is_move_constructible, T, E>::value)
        = delete;
        Result(Result&&)
            requires meta::All<std::is_trivially_move_constructible, T, E>::value
        = default;

        constexpr Result(Result&& other)
            noexcept(meta::All<std::is_nothrow_move_constructible, T, E>::value)
            requires meta::All<std::is_move_constructible, T, E>::value
            : m_has_value(other.m_has_value)
        {
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), std::move(other.m_value));
            }
            else {
                std::construct_at(std::addressof(m_error), std::move(other.m_error));
            }
        }

        auto operator=(Result const&) -> Result&
            requires(!meta::All<std::is_copy_assignable, T, E>::value)
        = delete;
        auto operator=(Result const&) -> Result&
            requires meta::All<std::is_trivially_copy_assignable, T, E>::value
        = default;

        constexpr auto operator=(Result const& other)
            noexcept(meta::All<Nothrow_copyable, T, E>::value) -> Result&
            requires meta::All<std::is_copy_constructible, T, E>::value
                  && (!meta::All<std::is_trivially_copy_assignable, T, E>::value)
        {
            if (this != &other) {
                if (m_has_value) {
                    if (other.m_has_value) {
                        copy_assign(m_value, other.m_value);
                    }
                    else {
                        reconstruct(*this, Error { other.m_error });
                        m_has_value = false;
                    }
                }
                else {
                    if (other.m_has_value) {
                        reconstruct(*this, other.m_value);
                        m_has_value = true;
                    }
                    else {
                        copy_assign(m_error, other.m_error);
                    }
                }
            }
            return *this;
        }

        auto operator=(Result&&) -> Result&
            requires(!std::is_move_assignable_v<T>)
        = delete;
        auto operator=(Result&&) noexcept -> Result&
            requires std::is_trivially_move_assignable_v<T>
        = default;

        constexpr auto operator=(Result&& other) noexcept -> Result&
            requires std::is_move_constructible_v<T> && (!std::is_trivially_move_assignable_v<T>)
        {
            if (this != &other) {
                if (m_has_value) {
                    if (other.m_has_value) {
                        move_assign(m_value, other.m_value);
                    }
                    else {
                        reconstruct(*this, Error { std::move(other.m_error) });
                        m_has_value = false;
                    }
                }
                else {
                    if (other.m_has_value) {
                        reconstruct(*this, std::move(other.m_value));
                        m_has_value = true;
                    }
                    else {
                        move_assign(m_error, other.m_error);
                    }
                }
            }
            return *this;
        }

        ~Result()
            requires(meta::All<std::is_trivially_destructible, T, E>::value)
        = default;

        constexpr ~Result()
            requires(!meta::All<std::is_trivially_destructible, T, E>::value)
        {
            if (m_has_value) {
                std::destroy_at(std::addressof(m_value));
            }
            else {
                std::destroy_at(std::addressof(m_error));
            }
        }

        constexpr auto reset() noexcept -> void
            requires std::is_nothrow_default_constructible_v<T>
        {
            if (m_has_value) {
                move_assign(m_value, T {});
            }
            else {
                std::destroy_at(std::addressof(m_error));
                std::construct_at(std::addressof(m_value));
                m_value = true;
            }
        }

        [[nodiscard]] constexpr auto has_value() const noexcept -> bool
        {
            return m_has_value;
        }

        [[nodiscard]] constexpr auto is_error() const noexcept -> bool
        {
            return !m_has_value;
        }

        [[nodiscard]] constexpr operator bool() const noexcept
        {
            return m_has_value;
        }

        template <class Self>
        [[nodiscard]] constexpr auto operator*(this Self&& self)
            noexcept(nothrow_deref) -> Qualified_like<Self, T>
        {
            Deref_config::validate_access(self.m_has_value);
            return std::forward_like<Self>(self.m_value);
        }

        [[nodiscard]] constexpr auto operator->(this auto&& self)
            noexcept(nothrow_deref) -> decltype(std::addressof(self.m_core.m_value))
        {
            Deref_config::validate_access(self.m_has_value);
            return std::addressof(self.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto unwrap(this Self&& self)
            noexcept(nothrow_unwrap) -> Qualified_like<Self, T>
        {
            Unwrap_config::validate_access(self.m_has_value);
            return std::forward_like<Self>(self.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto unwrap_unchecked(this Self&& self) noexcept
            -> Qualified_like<Self, T>
        {
            return std::forward_like<Self>(self.m_value);
        }

        template <class Self>
        [[nodiscard]] constexpr auto unwrap_err(this Self&& self)
            noexcept(nothrow_unwrap) -> Qualified_like<Self, E>
        {
            Unwrap_config::validate_access(!self.m_has_value);
            return std::forward_like<Self>(self.m_error);
        }

        template <class Self>
        [[nodiscard]] constexpr auto unwrap_err_unchecked(this Self&& self) noexcept
            -> Qualified_like<Self, E>
        {
            return std::forward_like<Self>(self.m_error);
        }

        template <class Self>
        [[nodiscard]] constexpr auto val(this Self&& self)
            noexcept(std::is_nothrow_constructible_v<T, Qualified_like<Self, T>>)
                -> Maybe<T, Unwrap_config, Deref_config>
        {
            if (!self.m_has_value) {
                return nothing;
            }
            return Maybe<T, Unwrap_config, Deref_config>(
                in_place, std::forward_like<Self>(self.m_value));
        }

        template <class Self>
        [[nodiscard]] constexpr auto err(this Self&& self)
            noexcept(std::is_nothrow_constructible_v<E, Qualified_like<Self, E>>)
                -> Maybe<E, Unwrap_config, Deref_config>
        {
            if (self.m_has_value) {
                return nothing;
            }
            return Maybe<E, Unwrap_config, Deref_config>(
                in_place, std::forward_like<Self>(self.m_error));
        }

        template <
            class Self,
            std::invocable<Qualified_like<Self, T>> Function,
            class R = std::invoke_result_t<Function&&, Qualified_like<Self, T>>>
        [[nodiscard]] constexpr auto map(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>)
                -> Result<R, E, Unwrap_config, Deref_config>
        {
            if (self.m_has_value) {
                return Result<R, E, Unwrap_config, Deref_config> { std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_value)) };
            }
            return Result<R, E, Unwrap_config, Deref_config> { Error<E> {
                std::forward_like<Self>(self.m_error) } };
        }

        template <class Self, std::invocable<Qualified_like<Self, T>> Function>
        constexpr auto map(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>) -> void
            requires std::is_void_v<std::invoke_result_t<Function&&, Qualified_like<Self, T>>>
        {
            if (self.m_has_value) {
                std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_value));
            }
        }

        template <
            class Self,
            std::invocable<Qualified_like<Self, E>> Function,
            class R = std::invoke_result_t<Function&&, Qualified_like<Self, E>>>
        [[nodiscard]] constexpr auto map_err(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, E>>)
                -> Result<T, R, Unwrap_config, Deref_config>
        {
            if (!self.m_has_value) {
                return Result<T, R, Unwrap_config, Deref_config> { Error<R> { std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_error)) } };
            }
            return Result<T, R, Unwrap_config, Deref_config> { std::forward_like<Self>(
                self.m_value) };
        }

        template <class Self, std::invocable<Qualified_like<Self, E>> Function>
        constexpr auto map_err(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, E>>) -> void
            requires std::is_void_v<std::invoke_result_t<Function&&, Qualified_like<Self, E>>>
        {
            if (!self.m_has_value) {
                std::invoke(
                    std::forward<Function>(function), std::forward_like<Self>(self.m_error));
            }
        }

        [[nodiscard]] constexpr auto ref() & noexcept
            -> Result<Ref<T>, Ref<E>, Unwrap_config, Deref_config>
        {
            if (m_has_value) {
                return Ref { m_value };
            }
            return Error { Ref { m_error } };
        }

        [[nodiscard]] constexpr auto ref() const& noexcept
            -> Result<Ref<T const>, Ref<E const>, Unwrap_config, Deref_config>
        {
            if (m_has_value) {
                return Ref { m_value };
            }
            return Error { Ref { m_error } };
        }

        auto ref() &&      = delete;
        auto ref() const&& = delete;

        // NOLINTEND(cppcoreguidelines-pro-type-union-access)
    };

} // namespace aa

namespace aa::inline basics {
    using aa::Result;
    using aa::Error;
} // namespace aa::inline basics
