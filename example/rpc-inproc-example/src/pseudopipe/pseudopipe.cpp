#include "pseudopipe.h"

namespace vsh::example {
    pseudopipe &pseudopipe::instance_cs()
    {
        static pseudopipe pp;
        return pp;
    }

    pseudopipe &pseudopipe::instance_sc()
    {
        static pseudopipe pp;
        return pp;
    }

    int pseudopipe::send(cl::buffer &&buff)
    {
        {
            std::lock_guard guard(m_mtx);
            m_queue.push(std::move(buff));
        }

        m_cv.notify_all();

        return 0;
    }

    int pseudopipe::recv(cl::buffer &buff)
    {
        std::unique_lock lock(m_mtx);
        if(!m_queue.empty()) {
            buff = std::move(m_queue.front());
            m_queue.pop();
        } else {
            m_cv.wait(lock, [this]() { return !m_queue.empty(); });

            buff = std::move(m_queue.front());
            m_queue.pop();
        }

        return 0;
    }
}