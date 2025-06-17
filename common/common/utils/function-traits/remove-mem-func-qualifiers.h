#pragma once

namespace vshalygin::common {
    namespace internal {
        template<typename MemberFunc>
        struct remove_mem_func_q_impl
        {
            static_assert(sizeof(MemberFunc) == 0,
                          "type is not resolved as member function");
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...)>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const volatile>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const &>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const &&>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const volatile &>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const volatile &&>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const volatile noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const & noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) const && noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) volatile>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) volatile &>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) volatile &&>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) volatile noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) volatile & noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) volatile && noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) &>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) & noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) &&>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) && noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args...) noexcept>
        {
            using type = Ret(Class::*)(Args...);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......)>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const volatile>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const &>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const &&>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const volatile &>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const volatile &&>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const volatile noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const & noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) const && noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) volatile>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) volatile &>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) volatile &&>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) volatile noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) volatile & noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) volatile && noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) &>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) & noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) &&>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) && noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };

        template<typename Ret, typename Class, typename...Args>
        struct remove_mem_func_q_impl<Ret(Class::*)(Args......) noexcept>
        {
            using type = Ret(Class::*)(Args......);
        };
    }

    template<typename MemberFunc>
    struct remove_mem_func_q
        : public internal::remove_mem_func_q_impl<MemberFunc>
    {};

    template<typename MemberFunc>
    using remove_mem_func_q_t = typename remove_mem_func_q<MemberFunc>::type;
}
