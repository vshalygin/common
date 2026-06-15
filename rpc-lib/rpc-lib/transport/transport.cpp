#include "transport.h"
#include "rpc-lib/pipe/ipipe.h"
#include <cassert>

namespace vshalygin::rpc {
    transport::transport(std::shared_ptr<ipipe> pipe)
        : m_pipe(std::move(pipe))
    {
        assert(m_pipe && m_pipe->is_connected());
    }

    transport::~transport()
    {
        try {
            stop();
        } catch (...) {
            //TODO log
            std::terminate();
        }
    }

    void transport::send_async(cl::buffer &&message,
                               std::function<void()> &&error_handler)
    {
        auto res = m_pipe->write_async(std::move(message),
                                       [eh = std::move(error_handler)](pipe_op_res res) {
                                           if(res == pipe_op_res::failed) {
                                               eh();
                                           }
                                       });

        if(!res) {
            throw std::runtime_error("write_async failed");
        }
    }

    void transport::recv_async(std::function<void(bool, cl::buffer &&)> &&handler)
    {
        auto r = m_pipe->read_async([handler = std::move(handler)](pipe_op_res res, cl::buffer &&msg) {
                                        if(is_success(res)) {
                                            handler(true, std::move(msg));
                                        } else {
                                            handler(false, {});
                                        }
                                    });

        if(!r) {
            stop();
        }
    }

    void transport::start(std::function<void()> &&start_callback, std::function<void()> &&stop_callback)
    {
        {
            std::lock_guard lock(m_mtx);
            if(m_state != state::init) {
                throw std::logic_error("pipe transport was started");
            }

            auto res = m_pipe->wait_connect_for(std::chrono::seconds(10));
            if(!res) {
                throw std::runtime_error("unable to wait pipe connection");
            }

            m_stop_callback = std::move(stop_callback);
            m_state = state::started;
        }

        if(start_callback) try {
            start_callback();
        } catch (...) {
            //TODO log
        }
    }

    void transport::stop()
    {
        std::unique_lock lock(m_mtx);
        if(m_state == state::started) {
            m_pipe->invalidate();
            m_state = state::stopped;
            auto stop_callback = std::move(m_stop_callback);
            lock.unlock();

            if(stop_callback) try {
                stop_callback();
            } catch(...) {
                //TODO log
            }
        }
    }

    bool transport::is_running() const
    {
        std::lock_guard lock(m_mtx);
        return m_state == state::started;
    }
}
