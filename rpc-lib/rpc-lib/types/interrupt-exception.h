#pragma once
#include <exception>

namespace vshalygin::rpc {
    class interrupt_exception
        : public std::exception
    {
    public:
        using exception::exception;
    };
}