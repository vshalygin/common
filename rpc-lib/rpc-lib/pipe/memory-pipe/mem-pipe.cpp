#include "mem-pipe.h"
#include "common-lib/syncronization/event/event.h"

#include <mutex>
#include <condition_variable>

namespace vshalygin::rpc {
    mem_pipe::mem_pipe(bool is_server)
        : m_is_server(is_server)
    {}

    mem_pipe::~mem_pipe()
    {
        try {
            invalidate();
        } catch(...) {
            //TODO log fatal
            std::terminate();
        }
    }

    void mem_pipe::set_buffers(std::shared_ptr<mem_buffers> mem_buffers)
    {
        {
            std::lock_guard guard(m_mtx);
            m_mem_buffers = std::move(mem_buffers);
        }

        m_cv.notify_all();
    }

    bool mem_pipe::is_connected() const
    {
        std::lock_guard guard(m_mtx);
        return m_mem_buffers &&
               m_mem_buffers->client_to_server.is_valid() &&
               m_mem_buffers->server_to_client.is_valid();
    }

    pipe_wait_res mem_pipe::wait_connect_for(const std::chrono::microseconds &mcs) const
    {
        auto now = std::chrono::steady_clock::now();
        std::unique_lock lock(m_mtx);
        auto r = m_cv.wait_until(lock, now + mcs,
                                [this]() {
                                    return m_is_invalidated || m_mem_buffers != nullptr;
                                });

        return r ? (m_is_invalidated ? pipe_wait_res::invalidated : pipe_wait_res::connected)
                 : pipe_wait_res::timeout;
    }

    pipe_wait_res mem_pipe::wait_connect() const
    {
        std::unique_lock lock(m_mtx);
        m_cv.wait(lock,[this]() {
                           return m_is_invalidated || m_mem_buffers != nullptr;
                       });

        return m_is_invalidated ? pipe_wait_res::invalidated : pipe_wait_res::connected;
    }

    bool mem_pipe::write_async(cl::buffer &&msg, std::function<void(pipe_op_res)> &&handler)
    {
        std::lock_guard guard(m_mtx);
        if(!m_mem_buffers) {
            return false;
        }
        auto &output_buff = m_is_server ?
                            m_mem_buffers->server_to_client :
                            m_mem_buffers->client_to_server;
        if(!output_buff.is_valid()) {
            return false;
        }

        output_buff.write_async(std::move(msg), std::move(handler));
        return true;
    }

    bool mem_pipe::read_async(std::function<void(pipe_op_res, cl::buffer &&)> &&handler)
    {
        std::lock_guard guard(m_mtx);
        if(!m_mem_buffers) {
            return false;
        }
        auto &input_buff = m_is_server ?
                           m_mem_buffers->client_to_server :
                           m_mem_buffers->server_to_client;

        if(!input_buff.is_valid()) {
            return false;
        }

        input_buff.read_async(std::move(handler));
        return true;
    }

    bool mem_pipe::try_to_write_for(cl::buffer &&msg, const std::chrono::microseconds &timeout)
    {
        struct data
        {
            cl::event sync_event;
            pipe_op_res res = pipe_op_res::failed;
        };
        auto d = std::make_shared<data>();
        auto task = [d](pipe_op_res r) {
            d->res = r;
            d->sync_event.set();
        };

        if(!write_async(std::move(msg), std::move(task))) {
            return false;
        }

        if(!d->sync_event.wait_for(timeout)) {
            return false;
        }

        return is_success(d->res);
    }

    std::optional<cl::buffer> mem_pipe::try_to_read_for(const std::chrono::microseconds &timeout)
    {
        struct data
        {
            cl::event sync_event;
            pipe_op_res res = pipe_op_res::failed;
            cl::buffer buf;
        };
        auto d = std::make_shared<data>();
        auto task = [d](pipe_op_res r, cl::buffer &&b) {
            d->res = r;
            d->buf = std::move(b);
            d->sync_event.set();
        };

        if(!read_async(std::move(task))) {
            return std::nullopt;
        }

        if(!d->sync_event.wait_for(timeout)) {
            return std::nullopt;
        }

        return is_success(d->res) ? std::optional<cl::buffer>(std::move(d->buf)) : std::nullopt;
    }

    void mem_pipe::invalidate()
    {
        {
            std::lock_guard lock(m_mtx);
            m_is_invalidated = true;

            if(m_mem_buffers) {
                m_mem_buffers->client_to_server.invalidate();
                m_mem_buffers->server_to_client.invalidate();
            }
        }

        m_cv.notify_all();
    }
}
