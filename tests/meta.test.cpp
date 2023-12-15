#include <aa/meta.hpp>
#include <tuple>

using namespace aa::meta;

static_assert(std::is_same_v<
              typename Apply<std::tuple, List<int, float, char>>::type,
              std::tuple<int, float, char>>);

static_assert(std::is_same_v<
              typename Map<std::tuple, List<int, float, char>>::type,
              List<std::tuple<int>, std::tuple<float>, std::tuple<char>>>);

static_assert(Any<std::is_reference, int&, float&>::value);
static_assert(Any<std::is_reference, int, float&>::value);
static_assert(!Any<std::is_reference, int, float>::value);

static_assert(All<std::is_reference, int&, float&>::value);
static_assert(!All<std::is_reference, int, float&>::value);
static_assert(!All<std::is_reference, int, float>::value);
