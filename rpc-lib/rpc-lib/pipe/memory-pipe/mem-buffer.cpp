#include "mem-buffer.h"

namespace vshalygin::rpc {
    std::shared_ptr<mem_buffer> mem_buffer::create(std::shared_ptr<cl::thread_pool> thread_pool)
    {
        return std::shared_ptr<mem_buffer>(new mem_buffer(std::move(thread_pool)));
    }

    mem_buffer::mem_buffer(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(std::move(thread_pool))
    {}

    mem_buffer::~mem_buffer()
    {
        invalidate();
    }

    mem_buffer::write_future mem_buffer::write_async(cl::buffer &&data)
    {
        auto promise = make_promise(m_thread_pool.get(),
                                    [](pipe_op_res res) { return res; });
        auto future = promise.get_future();

        m_thread_pool->post([self = shared_from_this(),
                            promise = std::move(promise),
                            data = std::move(data)]() mutable
        {
            promise.resolve(self->write(std::move(data)));
            self->resolve_read_promises();
        });
        
        return future;
    }
    
    mem_buffer::read_future mem_buffer::read_async()
    {
        auto promise = make_promise(m_thread_pool.get(),
                                    [](pipe_op_res res, cl::buffer b) { return ftuple(res, std::move(b)); });
        auto future = promise.get_future();


        m_thread_pool->post([self = shared_from_this(), promise = std::move(promise)]() mutable {
            self->read(std::move(promise));
        });

        return future;
    }

    void mem_buffer::invalidate()
    {
        std::lock_guard guard(m_mtx);
        if(m_is_valid) {
            m_is_valid = false;
            m_buffer = {};
            while(!m_read_promises.empty()) {
                auto promise = std::move(m_read_promises.front());
                m_read_promises.pop();
                promise.resolve(pipe_op_res::canceled, {});
            }
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
        std::lock_guard guard(m_mtx);
        return m_read_promises.size();
    }

    pipe_op_res mem_buffer::write(cl::buffer &&data)
    {
        std::lock_guard guard(m_mtx);

        if(!m_is_valid) {
            return pipe_op_res::failed;
        }

        try {
            m_buffer.push(std::move(data));
        } catch (...) {
            return pipe_op_res::failed;
        }

        return pipe_op_res::success;
    }

    void mem_buffer::resolve_read_promises()
    {
        std::lock_guard guard(m_mtx);

        if(!m_read_promises.empty()) {
            auto read_promise = std::move(m_read_promises.front());
            m_read_promises.pop();
            read_promise.resolve(pipe_op_res::success, std::move(m_buffer.front()));
            m_buffer.pop();
        }
    }

    void mem_buffer::read(read_promise promise)
    {
        std::lock_guard guard(m_mtx);

        if(!m_is_valid) {
            promise.resolve(pipe_op_res::failed, {});
            return;
        }

        if(!m_buffer.empty()) {
            auto msg = std::move(m_buffer.front());
            m_buffer.pop();
            promise.resolve(pipe_op_res::success, std::move(msg));
        } else {
            m_read_promises.push(std::move(promise));
        }
    }
}
