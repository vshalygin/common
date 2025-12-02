#include "closure-guard.h"

namespace vsh::rpc {
    class closure_guard::impl
    {
        using Closure = google::protobuf::Closure;

    public:
        explicit impl(Closure *closure)
            : m_closure(closure)
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        ~impl()
        {
            try {
                if(m_closure) {
                    m_closure->Run();
                }
            } catch (...) {
                //TODO log fatal
                std::terminate();
            }
        }

    private:
        Closure *m_closure;
    };

    closure_guard::closure_guard(Closure *closure)
        : m_impl(std::make_shared<impl>(closure))
    {}
}
