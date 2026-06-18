#include "transport.h"
#include "rpc-lib/pipe/ipipe.h"
#include <cassert>

namespace vshalygin::rpc {
    transport::transport(std::shared_ptr<ipipe> pipe,
                         std::function<void()> &&stop_callback)
        : m_pipe(std::move(pipe))
        , m_stop_callback(std::move(stop_callback))
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

    void transport::stop()
    {
        if(!m_is_stopped.exchange(true, std::memory_order_acq_rel)) {
            m_pipe->invalidate();

            if(m_stop_callback) try {
                m_stop_callback();
            } catch(...) {
                //TODO log
            }
        }
    }

    bool transport::is_running() const
    {
        return m_pipe->is_connected();
    }
}
