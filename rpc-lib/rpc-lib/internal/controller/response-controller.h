#pragma once
#include "response-controller-base.h"

#include <rpc-lib/types/response-result.h>

#include <common-lib/mpl/type-transform.h>

namespace vshalygin::rpc {
    template<typename Callback>
    class response_controller final
        : public response_controller_base
        , public google::protobuf::Closure
    {
        response_controller(Callback &&callback, uint64_t connection_id);

    public:
        static response_controller *create_on_heap(Callback &&callback, uint64_t connection_id);

        response_controller(response_controller &) = delete;
        response_controller &operator=(response_controller &) = delete;

        void Run() override;

        uint64_t get_connection_id() const override;

    private:
        response_result m_response_result = response_result::ok;
        cl::remove_type_qualifiers_t<Callback> m_callback;

        const uint64_t m_connection_id;
        const int m_uncaught_exceptions;
    };

    template<typename Callback>
    response_controller<Callback>::response_controller(Callback &&callback, uint64_t connection_id)
        : m_callback(std::move(callback))
        , m_uncaught_exceptions(std::uncaught_exceptions())
        , m_connection_id(connection_id)
    {}

    template<typename Callback>
    response_controller<Callback> *response_controller<Callback>::create_on_heap(Callback &&callback,
                                                                                 uint64_t connection_id)
    {
        return new response_controller(std::move(callback), connection_id);
    }

    template<typename Callback>
    void response_controller<Callback>::Run()
    {
        try {
            if(std::uncaught_exceptions() > m_uncaught_exceptions) {
                m_response_result = response_result::unknown_error;
            }
            m_callback(m_response_result);
        } catch(...) {
        }

        delete this;
    }

    template<typename Callback>
    uint64_t response_controller<Callback>::get_connection_id() const
    {
        return m_connection_id;
    }
}
