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

static_assert(std::is_convertible_v<aa::Ref<int>, aa::Ref<int const>>);
static_assert(!std::is_convertible_v<aa::Ref<int const>, aa::Ref<int>>);

static_assert(aa::access_config<aa::Access_config_checked>);
static_assert(aa::access_config<aa::Access_config_unchecked>);
static_assert(aa::sentinel_config<aa::Sentinel_config_default_for<int>, int>);
