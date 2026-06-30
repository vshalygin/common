#include <common-lib/synchronization/internal/ordered-lock/compare-ordered-lockable-tuples.h>

using namespace vshalygin::cl::internal;

namespace {
    template<size_t I>
    struct test
    {
        static constexpr size_t order = I;
    };
}

static_assert(compare_ordered_lockable_tuples_v<std::tuple<test<0>, test<1>>,
                                                std::tuple<test<2>, test<3>>>);
static_assert(compare_ordered_lockable_tuples_v<std::tuple<test<1>, test<0>>,
                                                std::tuple<test<3>, test<2>>>);
static_assert(compare_ordered_lockable_tuples_v<std::tuple<test<0>>,
                                                std::tuple<test<2>>>);

static_assert(!compare_ordered_lockable_tuples_v<std::tuple<test<2>, test<3>>,
                                                 std::tuple<test<0>, test<1>>>);
static_assert(!compare_ordered_lockable_tuples_v<std::tuple<test<0>, test<3>>,
                                                 std::tuple<test<1>, test<2>>>);
static_assert(!compare_ordered_lockable_tuples_v<std::tuple<test<0>, test<2>>,
                                                 std::tuple<test<1>, test<4>>>);
static_assert(!compare_ordered_lockable_tuples_v<std::tuple<test<2>, test<1>>,
                                                 std::tuple<test<2>, test<5>>>);
static_assert(!compare_ordered_lockable_tuples_v<std::tuple<test<2>>,
                                                 std::tuple<test<1>>>);
static_assert(!compare_ordered_lockable_tuples_v<std::tuple<test<0>>,
                                                 std::tuple<test<0>>>);
