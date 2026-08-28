#pragma once

#include <common-lib/utils/value-proxy.h>
#include <common-lib/mpl/type-transform.h>

#include <memory>
#include <optional>
#include <mutex>
#include <cassert>
#include <utility>

namespace vshalygin::cl::internal {
    template<typename T>
    class future_value_state
    {
        template<typename, typename>
        friend class future_controller;

        template<typename>
        friend class locked_fvalue;

        using value_proxy_t = value_proxy<add_lvalue_ref_to_value_t<T>>;

    public:
        future_value_state() = default;

        future_value_state(const future_value_state &) = delete;
        future_value_state &operator=(const future_value_state &) = delete;

        void set_value(value_proxy_t value)
        {
            std::lock_guard g(m_mtx);
            assert(!m_value);
            m_value.emplace(std::move(value));
        }

        template<typename Func>
        decltype(auto) with_locked_value(Func &&func)
        {
            std::lock_guard g(m_mtx);
            assert(m_value);
            return std::forward<Func>(func)(m_value->to_underlying());
        }

    private:
        mutable std::mutex m_mtx;
        std::optional<value_proxy_t> m_value;
    };
}
