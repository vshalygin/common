#pragma once
#include "common/utils/function-traits/funtion-traits.h"

#include <tuple>
#include <memory>

namespace vshalygin::common {
    namespace internal {
        template<typename Ret, typename TupleArgs>
        class icallable
        {
        public:
            using return_type = Ret;
            using args_tuple_type = TupleArgs;

            virtual ~icallable() = default;

            virtual return_type call(args_tuple_type&& tuple_args) const = 0;
        };

        template<typename Callable>
        class callable final
            : public icallable<typename function_traits<Callable>::ret,
                               typename function_traits<Callable>::args>
        {
        public:
            using base = icallable<typename function_traits<Callable>::ret,
                                   typename function_traits<Callable>::args>;

            explicit callable(Callable&& underlying)
                : underlying_(std::move(underlying))
            {}

            typename base::return_type call(typename base::args_tuple_type&& tuple_args) const override
            {
                return std::apply(underlying_, std::move(tuple_args));
            }

        private:
            Callable underlying_;
        };
    }

    template<typename Signature>
    class function;

    template<typename Ret, typename...Args>
    class function<Ret(Args...)> final
    {
        using icallable = internal::icallable<Ret, std::tuple<Args...>>;

    public:
        template<typename Callable>
        function(Callable&& callable)
            : callable_(std::make_unique<internal::callable<Callable>>(std::forward<Callable>(callable)))
        {}

        Ret operator()(Args...args) const
        {
            return callable_->call(std::make_tuple<Args>(std::forward<Args>(args)...));
        }

    private:
        std::unique_ptr<icallable> callable_;
    };
}
