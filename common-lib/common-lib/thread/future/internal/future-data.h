#pragma once
#include <memory>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class future_controller;

    template<typename T, typename ThreadPool>
    class future_data final
    {
        friend class future_controller<T, ThreadPool>;

        using controller_t = future_controller<T, ThreadPool>;

        explicit future_data(std::shared_ptr<controller_t> controller);

    public:
        future_data(const future_data &) = delete;
        future_data &operator=(const future_data &) = delete;

        future_data(future_data &&) = default;
        future_data &operator=(future_data &&) = default;

        template<typename Func>
        void apply(Func &&func) const;

    private:
        std::shared_ptr<controller_t> m_controller;
    };
}
