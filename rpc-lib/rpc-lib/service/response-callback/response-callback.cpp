#include "response-callback.h"

namespace vsh::rpc {
    response_callback::response_callback(callback_t &&callback)
        : m_callback(std::move(callback))
    {
        assert(m_callback);
    }

    response_callback *response_callback::create_on_heap(callback_t &&callback)
    {
        return new response_callback(std::move(callback));
    }

    void response_callback::Run()
    {
        try {
            m_callback(m_response_result);
        } catch(...) {
            //TODO safe log
        }

        delete this;
    }

    void response_callback::Reset()
    {
        assert(!"Reset function should never be called in current implementation");
    }

    bool response_callback::Failed() const
    {
        return is_fail(m_response_result);
    }

    std::string response_callback::ErrorText() const
    {
        return to_string(m_response_result);
    }

    void response_callback::StartCancel()
    {
        assert(!"StartCancel function should never be called in current implementation");
    }

    void response_callback::SetFailed(const std::string &reason)
    {
        assert(is_fail(response_result_from_string(reason)));
        m_response_result = response_result_from_string(reason);
    }

    bool response_callback::IsCanceled() const
    {
        return m_response_result == response_result::canceled;
    }

    void response_callback::NotifyOnCancel(Closure * /*callback*/)
    {
        assert(!"NotifyOnCancel function should never be called in current implementation");
    }
}
