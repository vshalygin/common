#include "pipe-buffer.h"
#include "common-lib/syncronization/event/event.h"

namespace vsh::cl {
    std::shared_ptr<pipe_buffer> pipe_buffer::create(std::shared_ptr<cl::thread_pool> thread_pool)
    {
        return std::make_shared<pipe_buffer>(std::move(thread_pool),
                                             creator());
    }

    pipe_buffer::pipe_buffer(std::shared_ptr<cl::thread_pool> thread_pool,
                             creator)
        : m_strand(thread_pool->create_strand())
    {
        assert(thread_pool->get_num() > 1);
    }

    pipe_buffer::~pipe_buffer()
    {
        try {
            set_inactive();
        } catch (...) {
            //TODO safe log
        }
    }

    void pipe_buffer::write_async(cl::buffer &&buf, write_callback_t &&callback)
    {
        auto task = [self = shared_from_this(),
                     buf = std::move(buf),
                     callback = std::move(callback)]() mutable {

            auto res = pipe_op_res::unknown_error;
            if(self->is_active()) {
                self->m_message_queue.push(std::move(buf));
                res = pipe_op_res::ok;
            } else {
                res = pipe_op_res::inactive;
            }

            if(callback) try {
                callback(res);
            } catch (...) {
                //TODO safe log
            }

            if(is_success(res)) {
                self->post_read_from_queue_if_possible();
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
            if(self->m_read_callback) {
                try {
                    self->m_read_callback(pipe_op_res::canceled, {});
                } catch(...) {
                    //TODO log
                }
                self->m_read_callback = read_callback_t();
            }

            if(self->is_active()) {
                self->m_read_callback = std::move(callback);
                self->read_from_queue_if_possible();
            } else {
                try {
                    callback(pipe_op_res::inactive, {});
                } catch(...) {
                    //TODO log
                }
            }
        };

        m_strand->post(std::move(task));
    }

    bool pipe_buffer::is_active() const
    {
        return m_is_active.load();
    }

    void pipe_buffer::set_active()
    {
        m_is_active.store(true);
    }

    void pipe_buffer::set_inactive()
    {
        m_is_active.store(false);
        cl::event sync_event;

        auto task = [this, &sync_event]() {
            if(m_read_callback) try {
                m_read_callback(pipe_op_res::canceled, {});
            } catch(...) {
                //TODO log
            }

            m_read_callback = read_callback_t();
            m_message_queue = std::queue<cl::buffer>();

            sync_event.set();
        };

        m_strand->post(std::move(task));

        sync_event.wait();
    }

    void pipe_buffer::post_read_from_queue_if_possible()
    {
        auto task = [self = shared_from_this()]() {
            self->read_from_queue_if_possible();
        };

        m_strand->post(std::move(task));
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
            callback(pipe_op_res::ok, std::move(buffer));
        } catch (...) {
            //TODO log
        }
    }
}
