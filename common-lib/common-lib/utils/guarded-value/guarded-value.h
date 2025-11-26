#pragma once
#include <mutex>
#include <utility>

namespace vsh::cl {
    template<typename T>
    class guarded_value final
    {
    public:
        guarded_value(T &&value)
            : value_(std::move(value))
        {}

        template<typename...Args>
        guarded_value(Args&&...args)
            : value_(std::forward<Args>(args)...)
        {}

        guarded_value(guarded_value &) = delete;
        guarded_value &operator=(guarded_value &) = delete;

        std::pair<std::unique_lock<std::mutex>, T &> get()
        {
            return { std::unique_lock(mtx_), value_ };
        }

        std::pair<std::unique_lock<std::mutex>, const T &> get() const
        {
            return { std::unique_lock(mtx_), value_ };
        }

    private:
        mutable std::mutex mtx_;
        T value_;
    };
}
