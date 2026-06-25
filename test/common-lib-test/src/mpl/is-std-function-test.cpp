#include "common-lib/mpl/type-traits.h"

using namespace vshalygin::cl;

namespace {
    class functor
    {
    public:
        void operator()() {}
    };
}

//TODO add more tests
static_assert(!is_std_function_v<int>);
static_assert(!is_std_function_v<void()>);
static_assert(!is_std_function_v<const functor &>);
static_assert(!is_std_function_v<functor>);
static_assert(!is_std_function_v<decltype([]() {})>);
static_assert(!is_std_function_v<void(*)()>);
static_assert(!is_std_function_v<decltype(&functor::operator())>);


static_assert(is_std_function_v<std::function<void(int &, char &&)>>);
static_assert(is_std_function_v<const std::function<void()> &>);
static_assert(is_std_function_v<std::function<const char &(double &&, const int &)> &&>);
static_assert(is_std_function_v<const volatile std::function<int &(double)> &&>);
static_assert(is_std_function_v<volatile std::function<void()> &&>);
