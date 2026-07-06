#pragma once
#include "rpc-lib/types/response-result.h"

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <functional>
#include <memory>

namespace vshalygin::rpc {
    template<typename Callback>
    class response_callback final
        : public google::protobuf::RpcController
        , public google::protobuf::Closure
    {
        response_callback(Callback &&callback);

    public:
        response_callback(response_callback &) = delete;
        response_callback &operator=(response_callback &) = delete;

        static response_callback *create_on_heap(Callback &&callback);

        void Run() override;

        void Reset() override;
        bool Failed() const override;
        std::string ErrorText() const override;
        void StartCancel() override;
        void SetFailed(const std::string &reason) override;
        bool IsCanceled() const override;
        void NotifyOnCancel(Closure * /*callback*/) override;

    private:
        response_result m_response_result = response_result::ok;
        Callback m_callback;

        const int m_uncaught_exceptions;
    };

    template<typename Callback>
    response_callback<Callback>::response_callback(Callback &&callback)
        : m_callback(std::move(callback))
        , m_uncaught_exceptions(std::uncaught_exceptions())
    {}

    template<typename Callback>
    response_callback<Callback> *response_callback<Callback>::create_on_heap(Callback &&callback)
    {
        return new response_callback(std::move(callback));
    }

    template<typename Callback>
    void response_callback<Callback>::Run()
    {
        try {
            if(std::uncaught_exceptions() > m_uncaught_exceptions) {
                m_response_result = response_result::unknown_error;
            }
            m_callback(m_response_result);
        } catch(...) {
            //TODO safe log
        }

        delete this;
    }

    template<typename Callback>
    void response_callback<Callback>::Reset()
    {
        assert(!"Reset function should never be called in current implementation");
    }

    template<typename Callback>
    bool response_callback<Callback>::Failed() const
    {
        return is_fail(m_response_result);
    }

    template<typename Callback>
    std::string response_callback<Callback>::ErrorText() const
    {
        return to_string(m_response_result);
    }

    template<typename Callback>
    void response_callback<Callback>::StartCancel()
    {
        assert(!"StartCancel function should never be called in current implementation");
    }

    template<typename Callback>
    void response_callback<Callback>::SetFailed(const std::string &reason)
    {
        assert(is_fail(response_result_from_string(reason)));
        m_response_result = response_result_from_string(reason);
    }

    template<typename Callback>
    bool response_callback<Callback>::IsCanceled() const
    {
        return m_response_result == response_result::canceled;
    }

    template<typename Callback>
    void response_callback<Callback>::NotifyOnCancel(Closure * /*callback*/)
    {
        assert(!"NotifyOnCancel function should never be called in current implementation");
    }
}
