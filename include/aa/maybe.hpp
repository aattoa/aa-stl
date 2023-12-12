#pragma once

#include <aa/utility.hpp>

namespace aa::dtl {
    template <sane, class>
    struct Maybe_core;

    template <sane T, class Config>
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

    template <sane T, class Config>
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

    struct Bad_maybe_access : std::exception {
        [[nodiscard]] auto what() const noexcept -> char const* override
        {
            return "aa::Bad_maybe_access";
        }
    };

    template <class Config, class T>
    concept maybe_config = requires(bool const has_value, T const& value) {
        {
            Config::validate_unwrap(has_value)
        } -> std::same_as<void>;
        {
            Config::validate_deref(has_value)
        } -> std::same_as<void>;
        {
            Config::sentinel_value()
        } -> one_of<T, void>;
        {
            Config::is_sentinel_value(value)
        } -> std::same_as<bool>;
    };

    // clang-format off

    template <bool check_value, bool check_deref>
    struct Basic_maybe_config {
        Basic_maybe_config() = delete;

        static constexpr auto validate_unwrap(bool const has_value) noexcept(!check_value) -> void
        {
            if constexpr (check_value) if (!has_value) throw Bad_maybe_access {};
        }
        static constexpr auto validate_deref(bool const has_value) noexcept(!check_deref) -> void
        {
            if constexpr (check_deref) if (!has_value) throw Bad_maybe_access {};
        }
        static auto sentinel_value() noexcept -> void;
        static constexpr auto is_sentinel_value(auto const&) noexcept -> bool { return false; }
    };

    // clang-format on

    using Maybe_config_checked         = Basic_maybe_config<true, true>;
    using Maybe_config_unchecked_deref = Basic_maybe_config<true, false>;
    using Maybe_config_unchecked       = Basic_maybe_config<false, false>;

    template <class>
    struct Maybe_config_default_for final : Maybe_config_checked {};

    template <class T>
    struct Maybe_config_default_for<Ref<T>> final : Maybe_config_checked {
        static constexpr auto sentinel_value() noexcept -> Ref<T>
        {
            return Ref<T>::unsafe_construct_null_reference();
        }

        static constexpr auto is_sentinel_value(Ref<T> const ref) noexcept -> bool
        {
            return ref.operator->() == nullptr;
        }
    };

    struct Nothing final : detail::Internal_tag_type_base {
        explicit consteval Nothing(detail::Internal_construct_tag) {}
    };

    inline constexpr Nothing nothing { detail::Internal_construct_tag {} };

    template <sane T, maybe_config<T> Config = Maybe_config_default_for<T>>
    class [[nodiscard]] Maybe final {
        dtl::Maybe_core<T, Config> m_core;

        static constexpr bool nothrow_value = noexcept(Config::validate_unwrap(bool {}));
        static constexpr bool nothrow_deref = noexcept(Config::validate_deref(bool {}));
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
            noexcept(nothrow_value) -> Qualified_like<Self, T>
        {
            Config::validate_unwrap(self.has_value());
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
            Config::validate_deref(self.has_value());
            return std::forward_like<Self>(self.m_core.m_value);
        }

        [[nodiscard]] constexpr auto operator->(this auto&& self) noexcept(nothrow_deref)
        {
            Config::validate_deref(self.has_value());
            return std::addressof(self.m_core.m_value);
        }

        template <class Self, std::invocable<Qualified_like<Self, T>> Function>
        [[nodiscard]] constexpr auto map(this Self&& self, Function&& function)
            noexcept(std::is_nothrow_invocable_v<Function&&, Qualified_like<Self, T>>)
                -> Maybe<std::invoke_result_t<Function&&, Qualified_like<Self, T>>>
        {
            if (!self.has_value()) {
                return {};
            }
            return Maybe<std::invoke_result_t<Function, Qualified_like<Self, T>>>(
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

        template <class Self>
        constexpr auto ref(this Self&& self)
            requires std::is_lvalue_reference_v<Self>
        {
            return self.map([](auto& ref) { return Ref { ref }; });
        }
    };

} // namespace aa

namespace aa::inline basics {
    using aa::Maybe;
    using aa::nothing;
} // namespace aa::inline basics
