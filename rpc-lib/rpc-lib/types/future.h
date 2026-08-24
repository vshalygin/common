#pragma once
#include <common-lib/thread/future/future.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    template<typename T>
    using future_data = cl::future_data<T, cl::thread_pool>;

    template<typename T>
    using future = cl::future<cl::thread_pool, T>;

    template<typename Signature>
    class promise
        : public cl::promise<cl::thread_pool, Signature>
    {
        using base = cl::promise<cl::thread_pool, Signature>;

    public:
        using base::base;
    };

    template<typename Function>
    promise(cl::thread_pool *thread_pool, Function &&function)
        -> promise<cl::function_signature_t<Function>>;

    using cl::ftuple;
}
