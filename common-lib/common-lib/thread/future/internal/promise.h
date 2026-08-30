#pragma once
#include "future-controller.h"
#include "is-future.h"
#include <atomic>
#include <cassert>
#include <exception>
#include <future>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T>
    class promise
    {
        static_assert(!is_future_v<T>,
                      "promise value type must not be a future");

    public:
        promise() = default;

        explicit promise(ThreadPool *thread_pool);

        promise(const promise &) = delete;
        promise &operator=(const promise &) = delete;

        promise(promise &&other) noexcept;
        promise &operator=(promise &&other) noexcept;

        ~promise() noexcept;

        template<typename U, typename TT = T,
                 std::enable_if_t<!std::is_void_v<TT>, int> = 0>
        void set_value(U &&value);

        template<typename TT = T,
                 std::enable_if_t<std::is_void_v<TT>, int> = 0>
        void set_value();

        void set_exception(std::exception_ptr exception);

        auto get_future();

        bool is_valid() const noexcept;

    private:
        void abandon() noexcept;
        void claim_completion();

    private:
        std::shared_ptr<future_controller<ThreadPool, T>> m_controller;
        future<ThreadPool, T> m_future;
        std::atomic_bool m_completion_pending{ false };
    };

    template<typename ThreadPool, typename T>
    promise<ThreadPool, T>::promise(ThreadPool *thread_pool)
        : m_controller(std::make_shared<future_controller<ThreadPool, T>>(thread_pool))
        , m_future(thread_pool, m_controller)
        , m_completion_pending(true)
    {
        assert(thread_pool);
        m_controller->set_self_shared_ptr(m_controller);
    }

    template<typename ThreadPool, typename T>
    promise<ThreadPool, T>::promise(promise &&other) noexcept
        : m_controller(std::move(other.m_controller))
        , m_future(std::move(other.m_future))
        , m_completion_pending(other.m_completion_pending.exchange(
              false,
              std::memory_order_acq_rel))
    {}

    template<typename ThreadPool, typename T>
    promise<ThreadPool, T> &
        promise<ThreadPool, T>::operator=(promise &&other) noexcept
    {
        if(this != &other) {
            abandon();

            m_controller = std::move(other.m_controller);
            m_future = std::move(other.m_future);
            m_completion_pending.store(
                other.m_completion_pending.exchange(false, std::memory_order_acq_rel),
                std::memory_order_release);
        }

        return *this;
    }

    template<typename ThreadPool, typename T>
    promise<ThreadPool, T>::~promise() noexcept
    {
        abandon();
    }

    template<typename ThreadPool, typename T>
    void promise<ThreadPool, T>::abandon() noexcept
    {
        if(!m_completion_pending.exchange(false, std::memory_order_acq_rel) ||
           !m_controller)
        {
            return;
        }

        m_controller->set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)));
    }

    template<typename ThreadPool, typename T>
    void promise<ThreadPool, T>::claim_completion()
    {
        if(!m_controller) {
            throw std::logic_error("promise is invalid");
        }

        if(!m_completion_pending.exchange(false, std::memory_order_acq_rel)) {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }
    }

    template<typename ThreadPool, typename T>
    template<typename U, typename TT,
             std::enable_if_t<!std::is_void_v<TT>, int>>
    void promise<ThreadPool, T>::set_value(U &&value)
    {
        static_assert(!is_future_v<U>,
                      "promise::set_value must not receive a future");

        claim_completion();

        try {
            m_controller->set_value(std::forward<U>(value));
        } catch(...) {
            m_controller->set_exception(std::current_exception());
            throw;
        }
    }

    template<typename ThreadPool, typename T>
    template<typename TT, std::enable_if_t<std::is_void_v<TT>, int>>
    void promise<ThreadPool, T>::set_value()
    {
        claim_completion();

        try {
            m_controller->set_value();
        } catch(...) {
            m_controller->set_exception(std::current_exception());
            throw;
        }
    }

    template<typename ThreadPool, typename T>
    void promise<ThreadPool, T>::set_exception(std::exception_ptr exception)
    {
        if(!exception) {
            throw std::invalid_argument("exception must not be null");
        }

        claim_completion();
        m_controller->set_exception(exception);
    }

    template<typename ThreadPool, typename T>
    auto promise<ThreadPool, T>::get_future()
    {
        if(!m_future.is_valid()) {
            throw std::logic_error("no future");
        }

        return std::move(m_future);
    }

    template<typename ThreadPool, typename T>
    bool promise<ThreadPool, T>::is_valid() const noexcept
    {
        return m_controller != nullptr;
    }
}
