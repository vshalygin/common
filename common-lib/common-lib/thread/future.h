#pragma once
#include "internal/future-impl.h"

#include <functional>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <memory>

namespace vshalygin::cl {
    class thread_pool;

    template<typename T, typename ThreadPool = thread_pool>
    using promise = internal::promise_impl<T, ThreadPool>;

    template<typename T, typename ThreadPool = thread_pool>
    using future = internal::future_impl<T, ThreadPool>;

    //Сделать определение типа через конструктор
}
