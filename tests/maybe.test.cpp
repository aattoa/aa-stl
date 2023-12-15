#include <aa/maybe.hpp>
#include <string>
#include "test_utility.hpp"

namespace {

    using namespace aa::basics;
    using namespace aa::tests;

    STATIC_TEST("Non-sentinel default construction", {
        return Maybe<Nontrivial> {}.is_empty() && Maybe<Nontrivial> { nothing }.is_empty();
    });

    STATIC_TEST("Sentinel default construction", {
        return Maybe<Nontrivial_with_sentinel> {}.is_empty()
            && Maybe<Nontrivial_with_sentinel> {}.unwrap_unchecked().integer
                   == Nontrivial_with_sentinel::sentinel;
    });

    STATIC_TEST("Non-sentinel in-place construction", {
        Maybe<Nontrivial> const a { 10 };
        return a.has_value() && a->integer == 10;
    });

    STATIC_TEST("Sentinel in-place construction", {
        Maybe<Nontrivial_with_sentinel> const a { Nontrivial { 10 } };
        return a.has_value() && a->integer == 10;
    });

    STATIC_TEST("Non-sentinel copy construction", {
        Maybe<Nontrivial> const a { 50 };
        Maybe<Nontrivial>       b { a };
        ++b.unwrap().integer;
        return (a->integer == 50) && (b->integer == 51);
    });

    STATIC_TEST("Sentinel copy construction", {
        Maybe<Nontrivial_with_sentinel> const a { Nontrivial { 50 } };
        Maybe<Nontrivial_with_sentinel>       b { a };
        ++b->integer;
        return (a->integer == 50) && (b->integer == 51);
    });

    STATIC_TEST("Non-sentinel move construction", {
        Maybe<Nontrivial>       a { 50 };
        Maybe<Nontrivial> const b { std::move(a) };
        return (a->integer == 0) && (b->integer == 50);
    });

    STATIC_TEST("Sentinel move construction", {
        Maybe<Nontrivial_with_sentinel>       a { Nontrivial { 50 } };
        Maybe<Nontrivial_with_sentinel> const b { std::move(a) };
        return (a->integer == 0) && (b->integer == 50);
    });

    STATIC_TEST("Non-sentinel copy assignment", {
        Maybe<Nontrivial> const a { 50 };
        Maybe<Nontrivial>       b { 10 };
        Maybe<Nontrivial> const c { b };
        b = a;
        return (a->integer == 50) && (b->integer == 50) && (c->integer == 10);
    });

    STATIC_TEST("Sentinel copy assignment", {
        Maybe<Nontrivial_with_sentinel> const a { Nontrivial { 50 } };
        Maybe<Nontrivial_with_sentinel>       b;
        b = a;
        return (a->integer == 50) && (b->integer == 50);
    });

    STATIC_TEST("Non-sentinel move assignment", {
        Maybe<Nontrivial>       a { 50 };
        Maybe<Nontrivial>       b { 10 };
        Maybe<Nontrivial> const c { b };
        b = std::move(a);
        return (a->integer == 0) && (b->integer == 50) && (c->integer == 10);
    });

    STATIC_TEST("Sentinel move assignment", {
        Maybe<Nontrivial_with_sentinel>       a { Nontrivial { 50 } };
        Maybe<Nontrivial_with_sentinel>       b { Nontrivial { 10 } };
        Maybe<Nontrivial_with_sentinel> const c { b };
        b = std::move(a);
        return (a->integer == 0) && (b->integer == 50) && (c->integer == 10);
    });

    STATIC_TEST("Non-sentinel reset", {
        Maybe<Nontrivial> a { 50 };
        Maybe<Nontrivial> b;
        a.reset();
        b.reset();
        return a.is_empty() && b.is_empty();
    });

    STATIC_TEST("Sentinel reset", {
        Maybe<Nontrivial_with_sentinel> a { Nontrivial { 50 } };
        Maybe<Nontrivial_with_sentinel> b;
        a.reset();
        b.reset();
        return (a.is_empty() && a.unwrap_unchecked() == Nontrivial_with_sentinel::sentinel)
            && (b.is_empty() && b.unwrap_unchecked() == Nontrivial_with_sentinel::sentinel);
    });

    STATIC_TEST("Non-sentinel emplace", {
        Maybe<Nontrivial> a;
        a.emplace(10);
        return a->integer == 10;
    });

    STATIC_TEST("Sentinel emplace", {
        Maybe<Nontrivial_with_sentinel> a;
        a.emplace(Nontrivial { 10 });
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
        return (a.map(square).unwrap() == 3.14) && (a->integer == 100)
            && (b.map(square).is_empty());
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
        { m.unwrap() }                -> std::same_as<Nontrivial&>;
        { c.unwrap() }                -> std::same_as<Nontrivial const&>;
        { std::move(m.unwrap()) }     -> std::same_as<Nontrivial&&>;
        { std::move(c.unwrap()) }     -> std::same_as<Nontrivial const&&>; // NOLINT: move from const

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

    static_assert(std::is_trivially_destructible_v<Maybe<int>>);
    static_assert(std::is_trivially_move_constructible_v<Maybe<int>>);
    static_assert(std::is_trivially_move_assignable_v<Maybe<int>>);
    static_assert(std::is_trivially_copy_constructible_v<Maybe<int>>);
    static_assert(std::is_trivially_copy_assignable_v<Maybe<int>>);
    static_assert(std::is_nothrow_destructible_v<Maybe<int>>);
    static_assert(std::is_nothrow_move_constructible_v<Maybe<int>>);
    static_assert(std::is_nothrow_move_assignable_v<Maybe<int>>);
    static_assert(std::is_nothrow_copy_constructible_v<Maybe<int>>);
    static_assert(std::is_nothrow_copy_assignable_v<Maybe<int>>);
    static_assert(!std::is_trivially_destructible_v<Maybe<std::string>>);
    static_assert(!std::is_trivially_move_constructible_v<Maybe<std::string>>);
    static_assert(!std::is_trivially_move_assignable_v<Maybe<std::string>>);
    static_assert(!std::is_trivially_copy_constructible_v<Maybe<std::string>>);
    static_assert(!std::is_trivially_copy_assignable_v<Maybe<std::string>>);
    static_assert(std::is_nothrow_destructible_v<Maybe<std::string>>);
    static_assert(std::is_nothrow_move_constructible_v<Maybe<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<Maybe<std::string>>);
    static_assert(!std::is_nothrow_copy_constructible_v<Maybe<std::string>>);
    static_assert(!std::is_nothrow_copy_assignable_v<Maybe<std::string>>);
    static_assert(std::is_copy_constructible_v<Maybe<std::string>>);
    static_assert(std::is_copy_assignable_v<Maybe<std::string>>);

    // In the common case, when `T` has no padding bytes, `Maybe<T>`
    // is larger than `T` because of the boolean state flag.
    static_assert(sizeof(Maybe<Nontrivial>) > sizeof(Nontrivial));

    // When `T` has a sentinel value, `Maybe<T>` is merely a value wrapper.
    static_assert(sizeof(Maybe<Nontrivial_with_sentinel>) == sizeof(Nontrivial_with_sentinel));

} // namespace
