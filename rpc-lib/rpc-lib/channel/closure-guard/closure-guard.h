#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>

namespace vsh::rpc {
    class closure_guard
    {
        using Closure = google::protobuf::Closure;

    public:
        explicit closure_guard(Closure *closure);
        
        closure_guard(const closure_guard &) = default;
        closure_guard &operator=(const closure_guard &) = default;

        closure_guard(closure_guard &&) = default;
        closure_guard &operator=(closure_guard &&) = default;

        void reset() noexcept;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
