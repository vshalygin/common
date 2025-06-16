#include "common/utils/function-traits/funtion-traits.h"
#include "common/utils/function/function.h"

#include <functional>
#include <iostream>

using namespace vshalygin::common;

int main()
{
    auto t = [](int) { std::cout << "Hello, world\n"; };
    function<void(int)> f(t);
    f(0);

    return 0;
}
