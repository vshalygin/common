#pragma once
#include <functional>

namespace vsh::cl {
    class istrand
    {
    public:
        virtual ~istrand() = default;

        virtual void post(std::function<void()> &&task) = 0;
    };
}
