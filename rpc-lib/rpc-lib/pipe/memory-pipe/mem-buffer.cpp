#include "mem-buffer.h"

#include <common-lib/thread/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    std::shared_ptr<mem_buffer> mem_buffer::create(cl::thread_pool *thread_pool)
    {
        return std::shared_ptr<mem_buffer>(new mem_buffer(thread_pool));
    }

    mem_buffer::mem_buffer(cl::thread_pool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_read_strand(m_thread_pool->get_io_context())
        , m_write_strand(m_thread_pool->get_io_context())
        , m_read_promises(std::make_shared<cl::value_locker<read_promise_container>>())
        , m_timer(m_thread_pool->get_io_context())
    {}

    mem_buffer::~mem_buffer()
    {
        invalidate(true);
    }

    mem_buffer::write_future mem_buffer::write_async(cl::buffer &&data,
                                                     const std::optional<std::chrono::milliseconds> &timeout)
    {
        std::lock_guard guard(m_mtx);
        if(!m_is_valid) {
            return write_future(m_thread_pool, pipe_op_res::failed);
        }

        cl::promise<cl::thread_pool, pipe_op_res> promise(m_thread_pool);
        auto future = promise.get_future();

        auto timeout_point = timeout ? std::chrono::steady_clock::now() + *timeout
                                     : std::chrono::steady_clock::time_point::max();
        m_write_strand.post([self = shared_from_this(),
                             promise = std::move(promise),
                             data = std::move(data),
                             timeout_point]() mutable
        {
            promise.set_value(self->write_impl(std::move(data), timeout_point));
            self->resolve_read_promise();
        });
        
        return future;
    }
    
    mem_buffer::read_future mem_buffer::read_async(const std::optional<std::chrono::milliseconds> &timeout)
    {
        read_promise promise(m_thread_pool);
        auto future = promise.get_future();

        std::lock_guard guard(m_mtx);
        const auto started_while_valid = m_is_valid;
        m_read_strand.post([self = shared_from_this(),
                            promise = std::move(promise),
                            timeout,
                            started_while_valid]() mutable {
            self->read_impl(std::move(promise), timeout, started_while_valid);
        });

        return future;
    }

    void mem_buffer::invalidate(bool cancel_read)
    {
        std::lock_guard guard(m_mtx);
        if(m_is_valid) {
            m_is_valid = false;
            m_read_invalidation_result = cancel_read ? pipe_op_res::canceled
                                                     : pipe_op_res::failed;
            m_buffer = {};
            auto read_promises = m_read_promises->lock();
            auto &q = read_promises->get<0>();
            for(auto it = q.begin(); it != q.end(); ++it) {
                q.modify(it, [cancel_read](read_promise_data &el) {
                    el.promise.set_value(cl::ftuple(
                        cancel_read ? pipe_op_res::canceled : pipe_op_res::failed,
                        cl::buffer{}));
                });
            }
            q.clear();
            m_timer.cancel_all();
        }
    }

    bool mem_buffer::is_valid() const
    {
        std::lock_guard guard(m_mtx);
        return m_is_valid;
    }

    size_t mem_buffer::get_pending_messages_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_buffer.size();
    }

    size_t mem_buffer::get_pending_read_handlers_count() const
    {
        return m_read_promises->lock()->get<0>().size();
    }

    pipe_op_res mem_buffer::write_impl(cl::buffer &&data, const std::chrono::steady_clock::time_point &timeout_point)
    {
        std::lock_guard guard(m_mtx);

        if(!m_is_valid) {
            return pipe_op_res::canceled;
        }

        if(std::chrono::steady_clock::now() > timeout_point) {
            return pipe_op_res::timeout;
        }

        try {
            m_buffer.push(std::move(data));
        } catch (...) {
            return pipe_op_res::failed;
        }

        return pipe_op_res::success;
    }

    void mem_buffer::resolve_read_promise()
    {
        read_promise to_delete;

        std::lock_guard guard(m_mtx);
        auto read_promises = m_read_promises->lock();
        auto &q = read_promises->get<0>();
        if(!q.empty() && !m_buffer.empty()) {
            auto buffer = std::move(m_buffer.front());
            m_buffer.pop();

            if(q.begin()->timer_id) {
                m_timer.cancel(*q.begin()->timer_id);
            }
            
            q.modify(q.begin(), [&buffer, &to_delete](read_promise_data &el) mutable {
                el.promise.set_value(cl::ftuple(pipe_op_res::success, std::move(buffer)));
                to_delete = std::move(el.promise);
            });
            q.pop_front();
        }
    }

    void mem_buffer::read_impl(read_promise promise,
                               const std::optional<std::chrono::milliseconds> &timeout,
                               bool started_while_valid)
    {
        std::lock_guard guard(m_mtx);

        if(!m_is_valid) {
            promise.set_value(cl::ftuple(
                started_while_valid ? m_read_invalidation_result : pipe_op_res::failed,
                cl::buffer{}));
        } else if(!m_buffer.empty()) {
            auto msg = std::move(m_buffer.front());
            m_buffer.pop();
            promise.set_value(cl::ftuple(pipe_op_res::success, std::move(msg)));
        } else {
            const auto id = m_next_read_promise_id++;

            auto read_promises = m_read_promises->lock();
            std::optional<uint64_t> timer_id;
            if(timeout) {
                auto timeout_callback = [id, read_promises_wp = std::weak_ptr(m_read_promises)]() {
                    if(auto read_promises = read_promises_wp.lock()) {
                        //avoid deadlock in case future callback stores mem_buffer itself
                        read_promise promise;
                        {
                            auto promises = read_promises->lock();
                            auto &m = promises->get<1>();
                            auto it = m.find(id);
                            if(it != m.end()) {
                                m.modify(it, [&promise](read_promise_data &el) {
                                    promise = std::move(el.promise);
                                });
                                m.erase(it);
                            }
                        }
                        if(promise.is_valid()) {
                            promise.set_value(cl::ftuple(pipe_op_res::timeout, cl::buffer{}));
                        }
                    }
                };

                timer_id = m_timer.start(std::move(timeout_callback), *timeout);
            }

            read_promises->push_back({ id, timer_id, std::move(promise) });
        }
    }
}
