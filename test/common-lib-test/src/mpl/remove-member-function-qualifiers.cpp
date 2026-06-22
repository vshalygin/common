#include <common-lib/mpl/type-transform.h>
#include <type_traits>

using namespace vshalygin::cl;

namespace {
    class test_class
    {
    public:
        void f0() {}
        void f1() noexcept {}
        void f2() const {}
        void f3() const noexcept {}
        void f4() volatile {}
        void f5() volatile noexcept {}
        void f6() const volatile {}
        void f7() const volatile noexcept {}
        void f8() & {}
        void f9() & noexcept {}
        void f10() const & {}
        void f11() const & noexcept {}
        void f12() volatile & {}
        void f13() volatile & noexcept {}
        void f14() const volatile & {}
        void f15() const volatile & noexcept {}
        void f16() && {}
        void f17() && noexcept {}
        void f18() const && {}
        void f19() const && noexcept {}
        void f20() volatile && {}
        void f21() volatile && noexcept {}
        void f22() const volatile && {}
        void f23() const volatile && noexcept {}
    };

    using expected = void(test_class:: *)();

    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f0)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f1)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f2)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f3)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f4)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f5)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f6)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f7)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f8)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f9)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f10)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f11)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f12)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f13)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f14)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f15)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f16)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f17)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f18)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f19)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f20)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f21)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f22)>>);
    static_assert(std::is_same_v<expected,
                  remove_member_function_qualifiers_t<decltype(&test_class::f23)>>);

}

