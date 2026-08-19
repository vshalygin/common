#include "common-lib/mpl/function-traits.h"

using namespace vshalygin::cl;

namespace {
    class functor
    {
    public:
        void operator()() {}
    };
}

static_assert(!is_std_function_v<int>);
static_assert(!is_std_function_v<void()>);
static_assert(!is_std_function_v<const functor &>);
static_assert(!is_std_function_v<functor>);
namespace { auto test_lambda = []() {}; }
static_assert(!is_std_function_v<decltype(test_lambda)>);
static_assert(!is_std_function_v<void(*)()>);
static_assert(!is_std_function_v<decltype(&functor::operator())>);

static_assert(is_std_function_v<std::function<void()>>);
static_assert(is_std_function_v<std::function<void(int &, char &&)>>);
static_assert(is_std_function_v<std::function<int &()>>);
static_assert(is_std_function_v<std::function<double &&(int &, char &&)>>);
static_assert(is_std_function_v<const std::function<void()>>);
static_assert(is_std_function_v<volatile std::function<void()>>);
static_assert(is_std_function_v<const volatile std::function<void()>>);
static_assert(is_std_function_v<const std::function<void()> &>);
static_assert(is_std_function_v<volatile std::function<void()> &>);
static_assert(is_std_function_v<const volatile std::function<void()> &>);
static_assert(is_std_function_v<const std::function<void()> &&>);
static_assert(is_std_function_v<volatile std::function<void()> &&>);
static_assert(is_std_function_v<const volatile std::function<void()> &&>);
