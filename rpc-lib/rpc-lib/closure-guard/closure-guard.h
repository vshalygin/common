#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>

namespace vshalygin::rpc {
    class closure_guard final
    {
        using Closure = google::protobuf::Closure;

    public:
        closure_guard() noexcept;
        explicit closure_guard(Closure *closure) noexcept;
        
        ~closure_guard();

        closure_guard(const closure_guard &) = delete;
        closure_guard &operator=(const closure_guard &) = delete;

        closure_guard(closure_guard &&) noexcept;
        closure_guard &operator=(closure_guard &&) noexcept;

    private:
        void reset() noexcept;

    private:
        Closure *m_closure;
    };
}
