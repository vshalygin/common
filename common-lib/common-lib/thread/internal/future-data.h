#pragma once
#include <memory>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class future_controller;

    template<typename T, typename ThreadPool>
    class future_data final
    {
        friend class future_controller<T, ThreadPool>;

        explicit future_data(std::shared_ptr<future_controller<T, ThreadPool>> controller)
            : m_controller(std::move(controller))
        {}

    public:
        future_data(const future_data &) = delete;
        future_data &operator=(const future_data &) = delete;

        future_data(future_data &&) = default;
        future_data &operator=(future_data &&) = default;

        template<typename Func>
        void apply(Func &&func) const;

        template<typename Func>
        void apply(Func &&func);

    private:
        std::shared_ptr<future_controller<T, ThreadPool>> m_controller;
    };
}
