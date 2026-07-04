#pragma once
#include <common-lib/thread/future/future.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    template<typename T>
    using future_data = cl::future_data<T, cl::thread_pool>;

    template<typename T>
    using future = cl::future<cl::thread_pool, T>;

    template<typename T, typename...ResolveArgs>
    using promise = cl::promise<cl::thread_pool, T, ResolveArgs...>;

    template<typename Func>
    auto make_promise(cl::thread_pool *thread_pool, Func &&func)
    {
        return cl::make_promise(thread_pool, std::forward<Func>(func));
    }

    template<typename...Args>
    using ftuple = cl::ftuple<Args...>;
}
