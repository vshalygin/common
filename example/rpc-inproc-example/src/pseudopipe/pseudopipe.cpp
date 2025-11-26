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
            std::lock_guard guard(mtx_);
            queue_.push(std::move(buff));
        }

        cv_.notify_all();

        return 0;
    }

    int pseudopipe::recv(cl::buffer &buff)
    {
        std::unique_lock lock(mtx_);
        if(!queue_.empty()) {
            buff = std::move(queue_.front());
            queue_.pop();
        } else {
            cv_.wait(lock, [this]() { return !queue_.empty(); });

            buff = std::move(queue_.front());
            queue_.pop();
        }

        return 0;
    }
}