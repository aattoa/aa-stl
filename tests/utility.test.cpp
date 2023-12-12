#include <aa/utility.hpp>

static_assert(std::is_same_v<aa::Qualified_like<int&, float>, float&>);
static_assert(std::is_same_v<aa::Qualified_like<int&, float&>, float&>);
static_assert(std::is_same_v<aa::Qualified_like<int&, float&&>, float&>);
static_assert(std::is_same_v<aa::Qualified_like<int&, float const>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int&, float const&>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int&, float const&&>, float const&>);

static_assert(std::is_same_v<aa::Qualified_like<int&&, float>, float&&>);
static_assert(std::is_same_v<aa::Qualified_like<int&&, float&>, float&&>);
static_assert(std::is_same_v<aa::Qualified_like<int&&, float&&>, float&&>);
static_assert(std::is_same_v<aa::Qualified_like<int&&, float const>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int&&, float const&>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int&&, float const&&>, float const&&>);

static_assert(std::is_same_v<aa::Qualified_like<int const&, float>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&, float&>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&, float&&>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&, float const>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&, float const&>, float const&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&, float const&&>, float const&>);

static_assert(std::is_same_v<aa::Qualified_like<int const&&, float>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&&, float&>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&&, float&&>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&&, float const>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&&, float const&>, float const&&>);
static_assert(std::is_same_v<aa::Qualified_like<int const&&, float const&&>, float const&&>);
