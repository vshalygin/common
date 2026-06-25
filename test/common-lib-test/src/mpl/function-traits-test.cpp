#include <common-lib/mpl/function-traits.h>
#include <functional>
#include <string>

using namespace vshalygin::cl;
//TODO add tests
namespace {
    template<typename T>
    class test_class
    {
    public:
        float &f(const int &, volatile double &&t, T)
        {
            static float v = 0;
            return v;
        }

        void f1()
        {}

        void f2() const
        {}

        void f3() const &
        {}

        void f4() const volatile & noexcept
        {}

        float &operator()(const int &, volatile double &&t, T)
        {
            static float v = 0;
            return v;
        }
    };

    template<typename T>
    class test_class1
    {
    public:
        float &operator()(const int &, volatile double &&t, T) const
        {
            static float v = 0;
            return v;
        }
    };

    template<typename T>
    class test_class2
    {
    public:
        int operator()(T) const & 
        {
            return 1;
        }
    };

    class test_class3
    {
    public:
        void operator()() const & noexcept
        {}
    };

    class test_class4
    {
    public:
        void operator()() const volatile &noexcept
        {}
    };
}

//member function
static_assert(std::is_same_v<function_arg_t<0, decltype(&test_class<char>::f)>, const int &>);
static_assert(std::is_same_v<function_arg_t<1, decltype(&test_class<char>::f)>, volatile double &&>);
static_assert(std::is_same_v<function_arg_t<2, decltype(&test_class<char>::f)>, char>);
static_assert(std::is_same_v<function_ret_t<decltype(&test_class<char>::f)>, float &>);
static_assert(std::is_same_v<function_class_t<decltype(&test_class<char>::f)>, test_class<char>>);
static_assert(std::is_same_v<function_args_as_tuple_t<decltype(&test_class<char>::f)>,
                             std::tuple<const int &, volatile double &&, char>>);
static_assert(function_arg_count_v<decltype(&test_class<char>::f)> == 3);

static_assert(std::is_same_v<function_ret_t<decltype(&test_class<char>::f1)>, void>);
static_assert(std::is_same_v<function_ret_t<decltype(&test_class<char>::f2)>, void>);
static_assert(std::is_same_v<function_ret_t<decltype(&test_class<char>::f3)>, void>);
static_assert(std::is_same_v<function_ret_t<decltype(&test_class<char>::f4)>, void>);


//functor
static_assert(std::is_same_v<function_arg_t<0, test_class<char>> &, const int &>);
static_assert(std::is_same_v<function_arg_t<1, test_class<char>>, volatile double &&>);
static_assert(std::is_same_v<function_arg_t<2, const test_class<char> &>, char>);
static_assert(std::is_same_v<function_ret_t<test_class<char> &&>, float &>);
static_assert(std::is_same_v<function_class_t< const volatile test_class<char> &&>, test_class<char>>);
static_assert(std::is_same_v<function_args_as_tuple_t<test_class<char>>,
    std::tuple<const int &, volatile double &&, char>>);
static_assert(function_arg_count_v<test_class<char>> == 3);

static_assert(std::is_same_v<function_ret_t<test_class1<char>>, float &>);
static_assert(std::is_same_v<function_ret_t<test_class2<char>>, int>);
static_assert(std::is_same_v<function_ret_t<test_class3>, void>);
static_assert(std::is_same_v<function_ret_t<test_class4>, void>);

//function
using test_function_t = float &(const int &, volatile double &&, char);
static_assert(std::is_same_v<function_arg_t<0, test_function_t>, const int &>);
static_assert(std::is_same_v<function_arg_t<1, test_function_t>, volatile double &&>);
static_assert(std::is_same_v<function_arg_t<2, test_function_t>, char>);
static_assert(std::is_same_v<function_ret_t<test_function_t>, float &>);
static_assert(std::is_same_v<function_args_as_tuple_t<test_function_t>,
    std::tuple<const int &, volatile double &&, char>>);
static_assert(function_arg_count_v<test_function_t> == 3);

using test_function_t1 = void() noexcept;
using test_function_t2 = void();
static_assert(std::is_same_v<function_ret_t<test_function_t1>, void>);
static_assert(std::is_same_v<function_ret_t<test_function_t2>, void>);

//function ptr
using test_function_ptr_t = float &(*)(const int &, volatile double &&, char);
static_assert(std::is_same_v<function_arg_t<0, test_function_ptr_t>, const int &>);
static_assert(std::is_same_v<function_arg_t<1, test_function_ptr_t>, volatile double &&>);
static_assert(std::is_same_v<function_arg_t<2, test_function_ptr_t>, char>);
static_assert(std::is_same_v<function_ret_t<test_function_ptr_t>, float &>);
static_assert(std::is_same_v<function_args_as_tuple_t<test_function_ptr_t>,
    std::tuple<const int &, volatile double &&, char>>);
static_assert(function_arg_count_v<test_function_ptr_t> == 3);

using test_function_ptr_t1 = void(*)() noexcept;
using test_function_ptr_t2 = void(*)();
static_assert(std::is_same_v<function_ret_t<test_function_ptr_t1>, void>);
static_assert(std::is_same_v<function_ret_t<test_function_ptr_t2>, void>);

//std::function
using std_function = const std::function<float &(const int &, volatile double &&, char)> &;
static_assert(std::is_same_v<function_arg_t<0, std_function>, const int &>);
static_assert(std::is_same_v<function_arg_t<1, std_function>, volatile double &&>);
static_assert(std::is_same_v<function_arg_t<2, std_function>, char>);
static_assert(std::is_same_v<function_ret_t<std_function>, float &>);
static_assert(std::is_same_v<function_class_t<std_function>,
                             std::function<float &(const int &, volatile double &&, char)>>);
static_assert(std::is_same_v<function_args_as_tuple_t<std_function>,
    std::tuple<const int &, volatile double &&, char>>);
static_assert(function_arg_count_v<std_function> == 3);

//lambda
using lambda = decltype([i = 1](const int &, volatile double &&, char)->int { return 0; });
static_assert(std::is_same_v<function_arg_t<0, lambda>, const int &>);
static_assert(std::is_same_v<function_arg_t<1, lambda>, volatile double &&>);
static_assert(std::is_same_v<function_arg_t<2, lambda>, char>);
static_assert(std::is_same_v<function_ret_t<lambda>, int>);
static_assert(std::is_same_v<function_args_as_tuple_t<lambda>,
    std::tuple<const int &, volatile double &&, char>>);
static_assert(function_arg_count_v<lambda> == 3);


//various checks
static_assert(function_arg_count_v<void()> == 0);
static_assert(std::is_same_v<function_arg_t<0, decltype([](std::string &&) {})>,
                             std::string &&>);
