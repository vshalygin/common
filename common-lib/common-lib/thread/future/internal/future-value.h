#pragma once
#include "future-value-state.h"
#include "is-future-tuple.h"

#include <common-lib/mpl/function-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/utils/type-qualifiers-cast.h>

#include <cassert>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace vshalygin::cl::internal {
    template<typename, typename>
    class future_controller;

    template<typename, typename>
    class future;

    template<typename, typename>
    class promise;

    template<typename T>
    class locked_fvalue
    {
        template<typename, typename>
        friend class fvalue;

        locked_fvalue(std::shared_ptr<future_value_state<T>> value_state)
            : m_value_state(std::move(value_state))
            , m_lock(*m_value_state->m_mtx)
        {}

    public:
        using value_type = std::remove_reference_t<T>;

        locked_fvalue(const locked_fvalue &) = delete;
        locked_fvalue &operator=(const locked_fvalue &) = delete;

        template<typename Func>
        decltype(auto) with(Func &&func)
        {
            using stored_type = remove_type_qualifiers_t<T>;

            if constexpr(is_future_tuple_v<stored_type>) {
                return ::vshalygin::cl::internal::apply(std::forward<Func>(func), **this);
            } else {
                static_assert(function_arg_count_v<Func> == 1,
                              "value callback must have 1 argument");
                using arg_t = function_arg_t<0, Func>;
                return std::forward<Func>(func)(type_qualifiers_cast<arg_t>(**this));
            }
        }

        std::add_pointer_t<value_type> operator->()
        {
            assert(m_value_state->m_value);
            auto &&value = m_value_state->m_value->to_underlying();
            return std::addressof(value);
        }

        std::add_pointer_t<std::add_const_t<value_type>> operator->() const
        {
            assert(m_value_state->m_value);
            auto &&value = m_value_state->m_value->to_underlying();
            return std::addressof(value);
        }

        decltype(auto) operator*()
        {
            assert(m_value_state->m_value);
            return m_value_state->m_value->to_underlying();
        }

        decltype(auto) operator*() const
        {
            assert(m_value_state->m_value);
            return m_value_state->m_value->to_underlying();
        }

    private:
        std::shared_ptr<future_value_state<T>> m_value_state;
        std::unique_lock<std::mutex> m_lock;
    };

    template<typename ThreadPool, typename T>
    class fvalue final
    {
        friend class future_controller<ThreadPool, T>;

        template<typename, typename>
        friend class future;

        template<typename, typename>
        friend class promise;

        explicit fvalue(std::shared_ptr<future_value_state<T>> value);

        auto get_value_state() const;
        void ensure_valid() const;

    public:
        fvalue(const fvalue &) = delete;
        fvalue &operator=(const fvalue &) = delete;

        fvalue(fvalue &&) = default;
        fvalue &operator=(fvalue &&) = default;

        auto lock();
        auto lock() const;

    private:
        std::shared_ptr<future_value_state<T>> m_value;
    };

    template<typename ThreadPool, typename T>
    fvalue<ThreadPool, T>::fvalue(std::shared_ptr<future_value_state<T>> value)
        : m_value(std::move(value))
    {}

    template<typename ThreadPool, typename T>
    auto fvalue<ThreadPool, T>::get_value_state() const
    {
        ensure_valid();
        return m_value;
    }

    template<typename ThreadPool, typename T>
    void fvalue<ThreadPool, T>::ensure_valid() const
    {
        if(!m_value) {
            throw std::logic_error("fvalue is invalid");
        }
    }

    template<typename ThreadPool, typename T>
    auto fvalue<ThreadPool, T>::lock()
    {
        ensure_valid();
        return locked_fvalue<T>(m_value);
    }

    template<typename ThreadPool, typename T>
    auto fvalue<ThreadPool, T>::lock() const
    {
        ensure_valid();
        return locked_fvalue<T>(m_value);
    }
}
