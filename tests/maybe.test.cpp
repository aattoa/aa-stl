#include <aa/maybe.hpp>
#include <string>

#define STATIC_TEST(name, ...) static_assert(([]() __VA_ARGS__)(), name);

namespace {

    struct Nontrivial {
        int integer;

        constexpr ~Nontrivial() {} // NOLINT: equals default

        explicit constexpr Nontrivial(int const value) : integer(value) {}

        constexpr Nontrivial(Nontrivial const& other) noexcept // NOLINT: equals default
            : integer(other.integer) {};

        constexpr Nontrivial(Nontrivial&& other) noexcept : integer(std::exchange(other.integer, 0))
        {}

        constexpr auto operator=(Nontrivial const& other) // NOLINT: equals default
            -> Nontrivial&
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

        [[nodiscard]] constexpr auto operator==(int const value) const noexcept -> bool
        {
            return integer == value;
        }
    };

    static_assert(
        !std::is_trivially_default_constructible_v<Nontrivial>
        && !std::is_trivially_copy_assignable_v<Nontrivial>
        && !std::is_trivially_copy_constructible_v<Nontrivial>
        && !std::is_trivially_move_assignable_v<Nontrivial>
        && !std::is_trivially_move_constructible_v<Nontrivial>
        && !std::is_trivially_destructible_v<Nontrivial>);

    using namespace aa::basics;

    STATIC_TEST("Default construction", {
        return Maybe<Nontrivial> {}.is_empty() && Maybe<Nontrivial> { nothing }.is_empty();
    });

    STATIC_TEST("In-place construction", {
        Maybe<Nontrivial> const a { 10 };
        return a.has_value() && a->integer == 10;
    });

    STATIC_TEST("Copy construction", {
        Maybe<Nontrivial> const a { 50 };
        Maybe<Nontrivial>       b { a };
        ++b.value().integer;
        return (a->integer == 50) && (b->integer == 51);
    });

    STATIC_TEST("Move construction", {
        Maybe<Nontrivial>       a { 50 };
        Maybe<Nontrivial> const b { std::move(a) };
        return (a->integer == 0) && (b->integer == 50);
    });

    STATIC_TEST("Copy assignment", {
        Maybe<Nontrivial> const a { 50 };
        Maybe<Nontrivial>       b { 10 };
        Maybe<Nontrivial> const c { b };
        b = a;
        return (a->integer == 50) && (b->integer == 50) && (c->integer == 10);
    });

    STATIC_TEST("Move assignment", {
        Maybe<Nontrivial>       a { 50 };
        Maybe<Nontrivial>       b { 10 };
        Maybe<Nontrivial> const c { b };
        b = std::move(a);
        return (a->integer == 0) && (b->integer == 50) && (c->integer == 10);
    });

    STATIC_TEST("reset", {
        Maybe<Nontrivial> a { 50 };
        Maybe<Nontrivial> b;
        a.reset();
        b.reset();
        return a.is_empty() && b.is_empty();
    });

    STATIC_TEST("emplace", {
        Maybe<Nontrivial> a;
        a.emplace(10);
        return a->integer == 10;
    });

    STATIC_TEST("non-void map const", {
        auto const square = [](Nontrivial const& nontrivial) {
            return Nontrivial(nontrivial.integer * nontrivial.integer);
        };
        Maybe<Nontrivial> const a { 10 };
        Maybe<Nontrivial> const b;
        return (a.map(square)->integer == 100) && (b.map(square).is_empty());
    });

    STATIC_TEST("non-void map mutate", {
        auto const square = [](Nontrivial& nontrivial) {
            nontrivial.integer *= nontrivial.integer;
            return 3.14;
        };
        Maybe<Nontrivial> a { 10 };
        Maybe<Nontrivial> b;
        return (a.map(square).value() == 3.14) && (a->integer == 100) && (b.map(square).is_empty());
    });

    STATIC_TEST("void map const", {
        int x {};

        Maybe<Nontrivial> const a;
        a.map([&x](Nontrivial const&) { ++x; });

        Maybe<Nontrivial> const b { 10 };
        b.map([&x](Nontrivial const&) { ++x; });

        return x == 1;
    });

    STATIC_TEST("void map mutate", {
        Maybe<Nontrivial> a;
        a.map([](Nontrivial& nontrivial) { ++nontrivial.integer; });

        Maybe<Nontrivial> b { 10 };
        b.map([](Nontrivial& nontrivial) { ++nontrivial.integer; });

        return a.is_empty() && b->integer == 11;
    });

    static_assert(requires(Maybe<Nontrivial> m, Maybe<Nontrivial> const c) {
        // clang-format off
        { m.value() }                 -> std::same_as<Nontrivial&>;
        { c.value() }                 -> std::same_as<Nontrivial const&>;
        { std::move(m.value()) }      -> std::same_as<Nontrivial&&>;
        { std::move(c.value()) }      -> std::same_as<Nontrivial const&&>; // NOLINT: move from const

        { *m }                        -> std::same_as<Nontrivial&>;
        { *std::move(m) }             -> std::same_as<Nontrivial&&>;
        { *c }                        -> std::same_as<Nontrivial const&>;
        { *std::move(c) }             -> std::same_as<Nontrivial const&&>; // NOLINT: move from const

        { m.operator->() }            -> std::same_as<Nontrivial*>;
        { std::move(m).operator->() } -> std::same_as<Nontrivial*>;
        { c.operator->() }            -> std::same_as<Nontrivial const*>;
        { std::move(c).operator->() } -> std::same_as<Nontrivial const*>; // NOLINT: move from const
        // clang-format on
    });

    static_assert(std::is_trivially_destructible_v<aa::Maybe<int>>);
    static_assert(std::is_trivially_move_constructible_v<aa::Maybe<int>>);
    static_assert(std::is_trivially_move_assignable_v<aa::Maybe<int>>);
    static_assert(std::is_trivially_copy_constructible_v<aa::Maybe<int>>);
    static_assert(std::is_trivially_copy_assignable_v<aa::Maybe<int>>);
    static_assert(std::is_nothrow_destructible_v<aa::Maybe<int>>);
    static_assert(std::is_nothrow_move_constructible_v<aa::Maybe<int>>);
    static_assert(std::is_nothrow_move_assignable_v<aa::Maybe<int>>);
    static_assert(std::is_nothrow_copy_constructible_v<aa::Maybe<int>>);
    static_assert(std::is_nothrow_copy_assignable_v<aa::Maybe<int>>);
    static_assert(!std::is_trivially_destructible_v<aa::Maybe<std::string>>);
    static_assert(!std::is_trivially_move_constructible_v<aa::Maybe<std::string>>);
    static_assert(!std::is_trivially_move_assignable_v<aa::Maybe<std::string>>);
    static_assert(!std::is_trivially_copy_constructible_v<aa::Maybe<std::string>>);
    static_assert(!std::is_trivially_copy_assignable_v<aa::Maybe<std::string>>);
    static_assert(std::is_nothrow_destructible_v<aa::Maybe<std::string>>);
    static_assert(std::is_nothrow_move_constructible_v<aa::Maybe<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<aa::Maybe<std::string>>);
    static_assert(!std::is_nothrow_copy_constructible_v<aa::Maybe<std::string>>);
    static_assert(!std::is_nothrow_copy_assignable_v<aa::Maybe<std::string>>);
    static_assert(std::is_copy_constructible_v<aa::Maybe<std::string>>);
    static_assert(std::is_copy_assignable_v<aa::Maybe<std::string>>);

} // namespace
