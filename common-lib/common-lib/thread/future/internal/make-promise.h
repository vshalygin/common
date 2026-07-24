#pragma once
#include "promise.h"
#include <common-lib/mpl/function-traits.h>

namespace vshalygin::cl::internal {
    template<typename Func, typename ThreadPool, size_t...ArgsIdx>
    auto do_make_promise_impl(ThreadPool *thread_pool, Func &&func, std::index_sequence<ArgsIdx...>)
    {
        return promise<ThreadPool, function_ret_t<Func>, function_arg_t<ArgsIdx, Func>...>(
                                                            thread_pool, std::forward<Func>(func));
    }

    template<typename Func, typename ThreadPool>
    auto do_make_promise(ThreadPool *thread_pool, Func &&func)
    {
        return do_make_promise_impl(thread_pool,
                                    std::forward<Func>(func),
                                    std::make_index_sequence<function_arg_count_v<Func>>());
    }
}
