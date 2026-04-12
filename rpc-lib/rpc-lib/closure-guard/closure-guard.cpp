#include "closure-guard.h"
#include <utility>

namespace vshalygin::rpc {
    closure_guard::closure_guard() noexcept
        : closure_guard(nullptr)
    {}

    closure_guard::closure_guard(Closure *closure) noexcept
        : m_closure(closure)
    {}

    closure_guard::~closure_guard()
    {
        reset();
    }

    closure_guard::closure_guard(closure_guard &&other) noexcept
        : closure_guard(nullptr)
    {
        std::swap(m_closure, other.m_closure);
    }

    closure_guard &closure_guard::operator=(closure_guard &&other) noexcept
    {
        if(this == &other) {
            return *this;
        }

        reset();
        std::swap(m_closure, other.m_closure);
        return *this;
    }

    void closure_guard::reset() noexcept
    {
        try {
            if(m_closure) {
                m_closure->Run();
                m_closure = nullptr;
            }
        } catch(...) {
            //TODO log fatal
            std::terminate();
        }
    }
}
