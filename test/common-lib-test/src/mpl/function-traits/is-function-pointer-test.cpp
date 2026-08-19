#include <common-lib/mpl/function-traits.h>
#include <functional>

using namespace vshalygin::cl;

namespace {
    class test
    {
    public:
        void f()
        {}
    };
}

static_assert(is_function_pointer_v<void(*)()>);
static_assert(is_function_pointer_v<int(*)()>);
static_assert(is_function_pointer_v<double &(*)(const int, const double &)>);

static_assert(is_function_pointer_v<double(*)(int)>);
static_assert(is_function_pointer_v<double(* const)(int)>);
static_assert(is_function_pointer_v<double(* volatile)(int)>);
static_assert(is_function_pointer_v<double(* const volatile)(int)>);
static_assert(is_function_pointer_v<double(* const &)(int)>);
static_assert(is_function_pointer_v<double(* const &&)(int)>);
static_assert(is_function_pointer_v<double(* volatile &)(int)>);
static_assert(is_function_pointer_v<double(* volatile &&)(int)>);
static_assert(is_function_pointer_v<double(* const volatile &)(int)>);
static_assert(is_function_pointer_v<double(* const volatile &&)(int)>);

static_assert(!is_function_pointer_v<int>);
static_assert(!is_function_pointer_v<decltype(&test::f)>);
namespace { auto test_lambda = []() {}; }
static_assert(!is_function_pointer_v<decltype(test_lambda)> );
static_assert(!is_function_pointer_v<decltype(test_lambda)> );
static_assert(!is_function_pointer_v<std::function<void()>>);
