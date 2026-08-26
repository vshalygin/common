#include <common-lib/mpl/function-traits.h>

#include <functional>
#include <type_traits>

using namespace vshalygin::cl;

namespace {
    using test_signature = long &(const int &, volatile double &&, char);

    template<typename Function, typename Signature>
    inline constexpr bool has_signature_v =
        std::is_same_v<function_signature_t<Function>, Signature>;

    template<typename Function, typename Signature>
    inline constexpr bool all_object_qualifiers_have_signature_v =
        has_signature_v<Function, Signature> &&
        has_signature_v<Function &, Signature> &&
        has_signature_v<Function &&, Signature> &&
        has_signature_v<const Function, Signature> &&
        has_signature_v<const Function &, Signature> &&
        has_signature_v<const Function &&, Signature> &&
        has_signature_v<volatile Function, Signature> &&
        has_signature_v<volatile Function &, Signature> &&
        has_signature_v<volatile Function &&, Signature> &&
        has_signature_v<const volatile Function, Signature> &&
        has_signature_v<const volatile Function &, Signature> &&
        has_signature_v<const volatile Function &&, Signature>;

    template<typename Function, typename Signature>
    inline constexpr bool all_function_references_have_signature_v =
        has_signature_v<Function, Signature> &&
        has_signature_v<Function &, Signature> &&
        has_signature_v<Function &&, Signature>;

    template<typename R>
    using nullary_function = R();

    template<typename R>
    inline constexpr bool preserves_return_type_v =
        has_signature_v<nullary_function<R>, nullary_function<R>>;

    [[maybe_unused]] long &test_function(const int &, volatile double &&, char)
    {
        static long l = 0;
        return l;
    }
    [[maybe_unused]] long &test_noexcept_function(const int &, volatile double &&, char) noexcept
    {
        static long l = 0;
        return l;
    }

    class member_functions
    {
    public:
        long &unqualified(const int &, volatile double &&, char);
        long &unqualified_noexcept(const int &, volatile double &&, char) noexcept;
        long &const_qualified(const int &, volatile double &&, char) const;
        long &const_noexcept(const int &, volatile double &&, char) const noexcept;
        long &volatile_qualified(const int &, volatile double &&, char) volatile;
        long &volatile_noexcept(const int &, volatile double &&, char) volatile noexcept;
        long &const_volatile(const int &, volatile double &&, char) const volatile;
        long &const_volatile_noexcept(const int &, volatile double &&, char) const volatile noexcept;
        long &lvalue(const int &, volatile double &&, char) &;
        long &lvalue_noexcept(const int &, volatile double &&, char) & noexcept;
        long &const_lvalue(const int &, volatile double &&, char) const &;
        long &const_lvalue_noexcept(const int &, volatile double &&, char) const & noexcept;
        long &volatile_lvalue(const int &, volatile double &&, char) volatile &;
        long &volatile_lvalue_noexcept(const int &, volatile double &&, char) volatile & noexcept;
        long &const_volatile_lvalue(const int &, volatile double &&, char) const volatile &;
        long &const_volatile_lvalue_noexcept(const int &, volatile double &&, char) const volatile & noexcept;
        long &rvalue(const int &, volatile double &&, char) &&;
        long &rvalue_noexcept(const int &, volatile double &&, char) && noexcept;
        long &const_rvalue(const int &, volatile double &&, char) const &&;
        long &const_rvalue_noexcept(const int &, volatile double &&, char) const && noexcept;
        long &volatile_rvalue(const int &, volatile double &&, char) volatile &&;
        long &volatile_rvalue_noexcept(const int &, volatile double &&, char) volatile && noexcept;
        long &const_volatile_rvalue(const int &, volatile double &&, char) const volatile &&;
        long &const_volatile_rvalue_noexcept(const int &, volatile double &&, char) const volatile && noexcept;
    };

    struct unqualified_functor
    {
        long &operator()(const int &, volatile double &&, char);
    };

    struct unqualified_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) noexcept;
    };

    struct const_functor
    {
        long &operator()(const int &, volatile double &&, char) const;
    };

    struct const_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) const noexcept;
    };

    struct volatile_functor
    {
        long &operator()(const int &, volatile double &&, char) volatile;
    };

    struct volatile_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) volatile noexcept;
    };

    struct const_volatile_functor
    {
        long &operator()(const int &, volatile double &&, char) const volatile;
    };

    struct const_volatile_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) const volatile noexcept;
    };

    struct lvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) &;
    };

    struct lvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) & noexcept;
    };

    struct const_lvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) const &;
    };

    struct const_lvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) const & noexcept;
    };

    struct volatile_lvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) volatile &;
    };

    struct volatile_lvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) volatile & noexcept;
    };

    struct const_volatile_lvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) const volatile &;
    };

    struct const_volatile_lvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) const volatile & noexcept;
    };

    struct rvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) &&;
    };

    struct rvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) && noexcept;
    };

    struct const_rvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) const &&;
    };

    struct const_rvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) const && noexcept;
    };

    struct volatile_rvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) volatile &&;
    };

    struct volatile_rvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) volatile && noexcept;
    };

    struct const_volatile_rvalue_functor
    {
        long &operator()(const int &, volatile double &&, char) const volatile &&;
    };

    struct const_volatile_rvalue_noexcept_functor
    {
        long &operator()(const int &, volatile double &&, char) const volatile && noexcept;
    };

    static_assert(all_object_qualifiers_have_signature_v<unqualified_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<unqualified_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<volatile_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<volatile_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_volatile_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_volatile_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<lvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<lvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_lvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_lvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<volatile_lvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<volatile_lvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_volatile_lvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_volatile_lvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<rvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<rvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_rvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_rvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<volatile_rvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<volatile_rvalue_noexcept_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_volatile_rvalue_functor, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<const_volatile_rvalue_noexcept_functor, test_signature>);

    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::unqualified), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::unqualified_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_qualified), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::volatile_qualified), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::volatile_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_volatile), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_volatile_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::lvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::lvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_lvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_lvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::volatile_lvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::volatile_lvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_volatile_lvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_volatile_lvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::rvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::rvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_rvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_rvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::volatile_rvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::volatile_rvalue_noexcept), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_volatile_rvalue), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<
                  decltype(&member_functions::const_volatile_rvalue_noexcept), test_signature>);

    using function_type = decltype(test_function);
    using noexcept_function_type = decltype(test_noexcept_function);
    using function_pointer = decltype(&test_function);
    using noexcept_function_pointer = decltype(&test_noexcept_function);

    static_assert(all_function_references_have_signature_v<function_type, test_signature>);
    static_assert(all_function_references_have_signature_v<noexcept_function_type, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<function_pointer, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<noexcept_function_pointer, test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<std::function<test_signature>, test_signature>);

    [[maybe_unused]] auto lambda = [](const int &, volatile double &&, char) -> long & {
        static long value = 0;
        return value;
    };
    [[maybe_unused]] auto mutable_lambda = [](const int &, volatile double &&, char) mutable -> long & {
        static long value = 0;
        return value;
    };
    [[maybe_unused]] auto noexcept_lambda = [](const int &, volatile double &&, char) noexcept -> long & {
        static long value = 0;
        return value;
    };
    [[maybe_unused]] auto mutable_noexcept_lambda = [](const int &, volatile double &&, char) mutable noexcept -> long & {
        static long value = 0;
        return value;
    };

    static_assert(all_object_qualifiers_have_signature_v<decltype(lambda), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<decltype(mutable_lambda), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<decltype(noexcept_lambda), test_signature>);
    static_assert(all_object_qualifiers_have_signature_v<decltype(mutable_noexcept_lambda), test_signature>);

    static_assert(preserves_return_type_v<void>);
    static_assert(preserves_return_type_v<int>);
    static_assert(preserves_return_type_v<int &>);
    static_assert(preserves_return_type_v<int &&>);
    static_assert(preserves_return_type_v<const int>);
    static_assert(preserves_return_type_v<const int &>);
    static_assert(preserves_return_type_v<const int &&>);
    static_assert(preserves_return_type_v<volatile int>);
    static_assert(preserves_return_type_v<volatile int &>);
    static_assert(preserves_return_type_v<volatile int &&>);
    static_assert(preserves_return_type_v<const volatile int>);
    static_assert(preserves_return_type_v<const volatile int &>);
    static_assert(preserves_return_type_v<const volatile int &&>);
}
