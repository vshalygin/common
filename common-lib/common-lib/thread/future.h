#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <memory>

namespace vshalygin::cl {
    template<typename T>
    class promise;
    template<typename T>
    class future;

    namespace internal {
        //TODO void specialization
        template<typename T>
        class control_block final
            : public std::enable_shared_from_this<control_block<T>>
        {
        public:
            explicit control_block(boost::asio::io_context &io_context)
                : m_io_context(io_context)
            {}

            control_block(const control_block &) = delete;
            control_block &operator=(const control_block &) = delete;

            void set_on_success(std::function<void(T &&)> &&func)
            {
                std::lock_guard guard(m_mtx);
                if(m_on_success) {
                    throw std::logic_error("success handler already set");
                }

                m_on_success = func;
                if(m_val) {
                    post_success();
                }
            }

            void set_on_fail(std::function<void(std::exception_ptr)> &&func)
            {
                std::lock_guard guard(m_mtx);
                if(m_on_fail) {
                    throw std::logic_error("fail handler already set");
                }

                m_on_fail = std::move(func);
                if(m_exception) {
                    post_fail();
                }
            }

            void set_value(T &&value)
            {
                {
                    std::lock_guard guard(m_mtx);
                    if(m_val || m_exception) {
                        throw std::logic_error("value or exeption already set");
                    }

                    m_val = std::move(value);
                    if(m_on_success) {
                        post_success();
                    }
                }

                m_cv.notify_all();
            }

            void set_exception(const std::exception_ptr &e)
            {
                {
                    std::lock_guard guard(m_mtx);
                    if(m_val || m_exception) {
                        throw std::logic_error("value or exeption already set");
                    }

                    m_exception = e;
                    if(m_on_fail) {
                        post_fail();
                    }
                }

                m_cv.notify_all();
            }

            const T &get() const
            {
                std::unique_lock lock(m_mtx);
                m_cv.wait(lock, [this]() { return m_val || m_exception; });
                if(m_exception) {
                    std::rethrow_exception(*m_exception);
                }

                return m_val.value();
            }

        private:
            void post_success()
            {
                boost::asio::post(m_io_context, [s = this->shared_from_this()]() {
                    s->m_on_success(*(std::move(s->m_val)));
                });
            }

            void post_fail()
            {
                boost::asio::post(m_io_context, [s = this->shared_from_this()]() {
                    s->m_on_fail((*s->m_exception));
                });
            }

        private:
            boost::asio::io_context &m_io_context;

            std::optional<T> m_val;
            std::optional<std::exception_ptr> m_exception;

            std::function<void(T &&)> m_on_success;
            std::function<void(std::exception_ptr)> m_on_fail;

            mutable std::mutex m_mtx;
            mutable std::condition_variable m_cv;
        };
    }

    template<typename T>
    class future final
    {
        friend class promise<T>;

        explicit future(boost::asio::io_context &io_context,
                        std::shared_ptr<internal::control_block<T>> controll_block);

    public:
        future() = default;
        future(const future &) = delete;
        future &operator=(const future &) = delete;
        future(future &&) = default;
        future &operator=(future &&) = default;

        const T &get() const;

        template<typename U>
        future<U> then(std::function<U(T &&)> &&task);

        void catched(std::function<void(std::exception_ptr)> &&task);

        bool is_valid() const;

    private:
        boost::asio::io_context &m_io_context;
        std::shared_ptr<internal::control_block<T>> m_control_block;
    };

    template<typename T>
    class promise final
    {
    public:
        explicit promise(boost::asio::io_context &io_context)
            : m_io_context(io_context)
            , m_control_block(std::make_shared<internal::control_block<T>>(io_context))
            , m_future(io_context, m_control_block)
        {}

        promise(const promise &) = delete;
        promise &operator=(const promise &) = delete;
        promise(promise &&) = default;
        promise &operator=(promise &&) = default;

        future<T> get_future()
        {
            if(!m_future.is_valid()) {
                throw std::logic_error("no future");
            }

            return std::move(m_future);
        }

        void set_value(T &&val)
        {
            m_control_block->set_value(std::move(val));
        }

        void set_exception(std::exception_ptr e)
        {
            m_control_block->set_exception(e);
        }

    private:
        boost::asio::io_context &m_io_context;
        std::shared_ptr<internal::control_block<T>> m_control_block;
        future<T> m_future;
    };

    template<typename T>
    future<T>::future(boost::asio::io_context &io_context,
                      std::shared_ptr<internal::control_block<T>> controll_block)
        : m_io_context(io_context)
        , m_control_block(std::move(controll_block))
    {}

    template<typename T>
    const T &future<T>::get() const
    {
        if(!m_control_block) {
            throw std::logic_error("future is invalid");
        }

        return m_control_block->get();
    }

    template<typename T>
    template<typename U>
    future<U> future<T>::then(std::function<U(T &&)> &&task)
    {
        auto promise0 = std::make_shared<promise<U>>(m_io_context);
        auto future = promise0->get_future();

        auto success = [promise0, task = std::move(task)](T &&val) {
            try {
                promise0->set_value(task(std::move(val)));
            } catch (...) {
                promise0->set_exception(std::current_exception());
            }
        };
        auto fail = [promise0](std::exception_ptr e) {
            promise0->set_exception(e);
        };

        m_control_block->set_on_success(std::move(success));
        m_control_block->set_on_fail(std::move(fail));

        return future;
    }

    template<typename T>
    void future<T>::catched(std::function<void(std::exception_ptr)> &&task)
    {
        m_control_block->set_on_fail(std::move(task));
    }

    template<typename T>
    bool future<T>::is_valid() const
    {
        return m_control_block != nullptr;
    }
}
