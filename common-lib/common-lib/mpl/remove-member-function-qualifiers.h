#pragma once

namespace vshalygin::cl {
    template<typename T>
    struct remove_member_function_qualifiers;

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...)>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) &>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) & noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) &&>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) && noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const &>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const &noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const &&>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const &&noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) volatile>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) volatile noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) volatile &>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) volatile &noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) volatile &&>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) volatile &&noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const volatile>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const volatile noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const volatile &>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const volatile &noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const volatile &&>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename C, typename R, typename... Args>
    struct remove_member_function_qualifiers<R(C:: *)(Args...) const volatile &&noexcept>
    {
        using type = R(C:: *)(Args...);
    };

    template<typename T>
    using remove_member_function_qualifiers_t =
        typename remove_member_function_qualifiers<T>::type;
}
