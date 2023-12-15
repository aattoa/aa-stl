#pragma once

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

} // namespace aa

namespace aa::inline basics {
    using aa::Ref;
}
