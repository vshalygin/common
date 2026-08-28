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
        future_value_state()
            : m_mtx(std::make_shared<std::mutex>())
        {}

        explicit future_value_state(std::shared_ptr<std::mutex> mtx)
            : m_mtx(std::move(mtx))
        {
            assert(m_mtx);
        }

        future_value_state(const future_value_state &) = delete;
        future_value_state &operator=(const future_value_state &) = delete;

        void set_value(value_proxy_t value)
        {
            std::lock_guard g(*m_mtx);
            assert(!m_value);
            m_value.emplace(std::move(value));
        }

    private:
        std::shared_ptr<std::mutex> m_mtx;
        std::optional<value_proxy_t> m_value;
    };
}
