#include "pipe-buffer.h"
#include "common-lib/syncronization/event/event.h"

namespace vshalygin::cl {
    std::shared_ptr<pipe_buffer> pipe_buffer::create(std::shared_ptr<cl::thread_pool> thread_pool)
    {
        return std::make_shared<pipe_buffer>(std::move(thread_pool),
                                             creator());
    }

    pipe_buffer::pipe_buffer(std::shared_ptr<cl::thread_pool> thread_pool,
                             creator)
        : m_strand(thread_pool->create_strand())
    {
        assert(thread_pool->get_num() > 1 && "may be deadlock");
    }

    pipe_buffer::~pipe_buffer()
    {
        try {
            disable();
        } catch (...) {
            //TODO safe log
        }
    }

    void pipe_buffer::write_async(cl::buffer &&buf, write_callback_t &&callback)
    {
        auto task = [self = shared_from_this(),
                     buf = std::move(buf),
                     callback = std::move(callback)]() mutable {

            auto res = pipe_result::unknown_error;
            if(self->is_enabled()) {
                self->m_message_queue.push(std::move(buf));
                res = pipe_result::ok;
            } else {
                res = pipe_result::disabled;
            }

            if(callback) try {
                callback(res);
            } catch (...) {
                //TODO safe log
            }

            if(is_success(res)) {
                self->read_from_queue_if_possible();
            }

        };

        m_strand->post(std::move(task));
    }

    void pipe_buffer::read_async(read_callback_t &&callback)
    {
        if(!callback) {
            throw std::invalid_argument("read callback is empty");
        }

        auto task = [self = shared_from_this(), callback = std::move(callback)]() {
            if(self->m_read_callback) try {
                self->m_read_callback(pipe_result::canceled, {});
            } catch(...) {
                //TODO log
            }
            self->m_read_callback = read_callback_t();

            if(self->is_enabled()) {
                self->m_read_callback = std::move(callback);
                self->read_from_queue_if_possible();
            } else try {
                callback(pipe_result::disabled, {});
            } catch(...) {
                //TODO log
            }
        };

        m_strand->post(std::move(task));
    }

    bool pipe_buffer::is_enabled() const
    {
        return m_is_enabled.load(std::memory_order_acquire);
    }

    void pipe_buffer::enable()
    {
        m_is_enabled.store(true, std::memory_order_release);
    }

    void pipe_buffer::disable()
    {
        if(!m_is_enabled.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        event sync_event;

        auto task = [this, &sync_event]() {
            if(m_read_callback) try {
                m_read_callback(pipe_result::canceled, {});
            } catch(...) {
                //TODO log
            }

            m_read_callback = read_callback_t();
            m_message_queue = std::queue<cl::buffer>();

            sync_event.set();
        };

        if(m_strand->is_in_executing_context()) {
            task();
        } else {
            m_strand->post(std::move(task));
        }

        sync_event.wait();
    }

    size_t pipe_buffer::get_message_queue_count() const
    {
        cl::event sync_event;
        size_t ans = 0;
        auto task = [&]() {
            ans = m_message_queue.size();
            sync_event.set();
        };
        if(m_strand->is_in_executing_context()) {
            task();
        } else {
            m_strand->post(std::move(task));
        }
        sync_event.wait();

        return ans;
    }

    void pipe_buffer::read_from_queue_if_possible()
    {
        if(m_message_queue.empty() || !m_read_callback) {
            return;
        }

        read_callback_t callback;
        callback.swap(m_read_callback);

        cl::buffer buffer = std::move(m_message_queue.front());
        m_message_queue.pop();
        
        try {
            callback(pipe_result::ok, std::move(buffer));
        } catch (...) {
            //TODO safe log
        }
    }
}
