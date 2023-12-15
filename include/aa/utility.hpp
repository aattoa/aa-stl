#pragma once

#include <source_location>
#include <type_traits>
#include <concepts>
#include <utility>
#include <memory>

namespace aa::detail {
    struct Internal_tag_type_base {};
    struct Internal_construct_tag final {
        explicit consteval Internal_construct_tag() = default;
    };

    template <template <class...> class, class>
    struct Is_specialization_of : std::false_type {};
    template <template <class...> class F, class... Args>
    struct Is_specialization_of<F, F<Args...>> : std::true_type {};
} // namespace aa::detail

namespace aa {

    struct In_place final : detail::Internal_tag_type_base {
        explicit consteval In_place(detail::Internal_construct_tag) {}
    };
    inline constexpr In_place in_place { detail::Internal_construct_tag {} };

    template <class>
    struct In_place_type final : detail::Internal_tag_type_base {
        explicit consteval In_place_type(detail::Internal_construct_tag) {}
    };
    template <class T>
    constexpr In_place_type<T> in_place_type { detail::Internal_construct_tag {} };

    template <class T>
    concept tag_type = std::is_base_of_v<detail::Internal_tag_type_base, T>;

    // Instances of sane types can be reasonably stored in containers, with no surprising behavior.
    template <class T>
    concept sane = requires {
        requires !tag_type<T>;
        requires std::is_object_v<T>;
        requires std::is_nothrow_destructible_v<T>;
        requires std::is_nothrow_move_constructible_v<T> || !std::is_move_constructible_v<T>;
        requires std::is_nothrow_move_assignable_v<T> || !std::is_move_assignable_v<T>;
        requires std::is_nothrow_swappable_v<T> || !std::is_swappable_v<T>;
        requires std::is_trivially_destructible_v<T> || !std::is_trivially_copyable_v<T>;
    };

    template <class T>
    struct Nothrow_copyable
        : std::bool_constant<
              std::is_nothrow_copy_constructible_v<T>
              && (!std::is_copy_assignable_v<T> || std::is_nothrow_copy_assignable_v<T>)> {};
    template <class T>
    concept nothrow_copyable = Nothrow_copyable<T>::value;

    template <class T>
    struct Nothrow_movable
        : std::bool_constant<
              std::is_nothrow_move_constructible_v<T>
              && (!std::is_move_assignable_v<T> || std::is_nothrow_move_assignable_v<T>)> {};
    template <class T>
    concept nothrow_movable = Nothrow_movable<T>::value;

    template <class T, class... Ts>
    concept one_of = std::disjunction_v<std::is_same<T, Ts>...>;

    template <class T, template <class...> class F>
    concept specialization_of = detail::Is_specialization_of<F, T>::value;

    template <class T, class U>
    using Qualified_like = decltype(std::forward_like<T>(std::declval<U&>()));

    template <class T>
    class Ref final {
        T* m_pointer;
    public:
        constexpr Ref(T& reference) noexcept : m_pointer { std::addressof(reference) } {}

        constexpr Ref(Ref<std::remove_const_t<T>> const other) noexcept
            requires std::is_const_v<T>
            : m_pointer { other.m_pointer }
        {}

        [[nodiscard]] constexpr operator T&() const noexcept
        {
            return *m_pointer;
        }
        [[nodiscard]] constexpr auto operator*() const noexcept -> T&
        {
            return *m_pointer;
        }
        [[nodiscard]] constexpr auto operator->() const noexcept -> T*
        {
            return m_pointer;
        }
        [[nodiscard]] constexpr auto get() const noexcept -> T&
        {
            return *m_pointer;
        }

        [[nodiscard]] constexpr auto as_const() const noexcept -> Ref<T const>
        {
            return Ref<T const> { *this };
        }

        // Dangerous escape hatch for special cases, such as sentinel values.
        [[nodiscard]] static constexpr auto unsafe_construct_null_reference() noexcept -> Ref
        {
            return Ref { Null_construct_tag {} };
        }
    private:
        struct Null_construct_tag {};
        explicit constexpr Ref(Null_construct_tag) noexcept : m_pointer { nullptr } {}
    };

    template <class T, class... Args>
    constexpr auto reconstruct(T& object, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> void
        requires std::is_constructible_v<T, Args&&...> && std::is_nothrow_destructible_v<T>
              && (std::is_nothrow_constructible_v<T, Args && ...>
                  || std::is_nothrow_move_constructible_v<T>)
    {
        if constexpr (std::is_nothrow_constructible_v<T, Args&&...>) {
            std::destroy_at(std::addressof(object));
            std::construct_at(std::addressof(object), std::forward<Args>(args)...);
        }
        else {
            T backup = std::move(object);
            std::destroy_at(std::addressof(object));
            try {
                std::construct_at(std::addressof(object), std::forward<Args>(args)...);
            }
            catch (...) {
                std::construct_at(std::addressof(object), std::move(backup));
                throw;
            }
        }
    }

    template <sane T>
    constexpr auto move_assign(T& to, T&& from) noexcept -> void
        requires(std::is_move_constructible_v<T> || std::is_move_assignable_v<T>)
    {
        if constexpr (std::is_move_assignable_v<T>) {
            to = std::move(from); // NOLINT: bugprone move
        }
        else {
            reconstruct(to, std::move(from)); // NOLINT: bugprone move
        }
    }

    template <sane T>
    constexpr auto copy_assign(T& to, T const& from) noexcept(nothrow_copyable<T>) -> void
        requires(std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>)
    {
        if constexpr (std::is_copy_assignable_v<T>) {
            to = from;
        }
        else {
            reconstruct(to, from);
        }
    }

    struct Bad_access : std::exception {
        std::source_location source_location;

        Bad_access(std::source_location const location) noexcept : source_location { location } {}

        [[nodiscard]] auto what() const noexcept -> char const* override
        {
            return "aa::Bad_access";
        }
    };

    template <class Config, class T>
    concept sentinel_config = requires(T const& value) {
        {
            Config::sentinel_value()
        } -> one_of<T, void>;
        {
            Config::is_sentinel_value(value)
        } noexcept -> std::same_as<bool>;
    };

    template <class Config>
    concept access_config = requires(bool const has_value) {
        {
            Config::validate_access(has_value)
        } -> std::same_as<void>;
    };

    template <bool do_check>
    struct Basic_access_config {
        Basic_access_config() = delete;
        static constexpr auto validate_access(
            bool const                 has_value,
            std::source_location const caller = std::source_location::current())
            noexcept(!do_check) -> void
        {
            if constexpr (do_check) {
                if (!has_value) {
                    throw Bad_access { caller };
                }
            }
        }
    };

    struct Access_config_checked   final : Basic_access_config<true> {};
    struct Access_config_unchecked final : Basic_access_config<false> {};

    template <class T>
    struct Sentinel_config_default_for final {
        Sentinel_config_default_for() = delete;
        // Not implemented
        static auto sentinel_value() noexcept -> void;
        // Not implemented
        static auto is_sentinel_value(T const&) noexcept -> bool;
    };

    template <class T>
    struct Sentinel_config_default_for<Ref<T>> final {
        Sentinel_config_default_for() = delete;
        static constexpr auto sentinel_value() noexcept -> Ref<T>
        {
            return Ref<T>::unsafe_construct_null_reference();
        }
        static constexpr auto is_sentinel_value(Ref<T> const ref) noexcept -> bool
        {
            return ref.operator->() == nullptr;
        }
    };

} // namespace aa

namespace aa::inline basics {
    using aa::Ref;
}
