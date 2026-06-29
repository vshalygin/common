#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    struct value_comparator
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return T::value < U::value;
        }
    };

    struct value_comparator2
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return T::value > U::value;
        }
    };

    class test1
    {
    public:
        static constexpr size_t value = 34;

        int val = 0;
    };

    class test2
    {
    public:
        static constexpr size_t value = 40;

        int val = 4;
    };

    class test3
    {
    public:
        static constexpr size_t value = 50;

        int val = 5;
    };

    class test4
    {
    public:
        static constexpr size_t value = 70;

        int val = 6;
    };

    class move_only_test1
    {
    public:
        static constexpr size_t value = 10;

        explicit move_only_test1(int v)
            : val(v)
        {
        }

        move_only_test1(const move_only_test1 &) = delete;
        move_only_test1 &operator=(const move_only_test1 &) = delete;

        move_only_test1(move_only_test1 &&other)
        {
            val = other.val;
            other.val = 0;
        }

        int val = 6;
    };

    class move_only_test2
    {
    public:
        static constexpr size_t value = 10;

        explicit move_only_test2(int v)
            : val(v)
        {
        }

        move_only_test2(const move_only_test2 &) = delete;
        move_only_test2 &operator=(const move_only_test2 &) = delete;

        move_only_test2(move_only_test2 &&other)
        {
            val = other.val;
            other.val = 0;
        }

        int val = 10;
    };

    class move_only_counter
    {
    public:
        static constexpr size_t value = 10;

        move_only_counter() = default;

        move_only_counter(move_only_counter &) = delete;
        move_only_counter &operator=(move_only_counter &) = delete;

        move_only_counter(move_only_counter &&)
        {
            move_count++;
        }

        move_only_counter &operator=(move_only_counter &&)
        {
            move_assign_count++;
        }

        inline static void clear()
        {
            move_count = 0;
            move_assign_count = 0;
        }

        inline static int move_count = 0;
        inline static int move_assign_count = 0;
    };

    class copy_only_counter
    {
    public:
        static constexpr size_t value = 10;

        copy_only_counter() = default;

        copy_only_counter(const copy_only_counter &)
        {
            ++copy_count;
        }

        copy_only_counter &operator=(copy_only_counter &)
        {
            ++copy_assign_count;
        }

        inline static void clear()
        {
            copy_count = 0;
            copy_assign_count = 0;
        }

        inline static int copy_count = 0;
        inline static int copy_assign_count = 0;
    };
}

TEST(SortTuple2, BasicTest)
{
    std::tuple<test1, test2, test3> t{};

    auto s = sort_tuple2<value_comparator>(t); s;

    static_assert(std::is_same_v<decltype(s), std::tuple<test1, test2, test3>>);
}

TEST(SortTuple2, SortBasedOnSortCriterionTupleWithAnyTypeQualifiers)
{
    std::tuple<test3, test1, test2> t1{};
    const std::tuple<test3, test1, test2> t2{};
    std::tuple<test3, test1, test2> &t3 = t1;
    const std::tuple<test3, test1, test2> &t4 = t2;
    std::tuple<test3, test1, test2> &&t5 = std::move(t1);
    const std::tuple<test3, test1, test2> &&t6 = std::move(t1);

    auto s1 = sort_tuple2<value_comparator>(std::tuple<test3, test1, test2>{});
    auto s2 = sort_tuple2<value_comparator>(t1);
    auto s3 = sort_tuple2<value_comparator>(std::move(t1));
    auto s4 = sort_tuple2<value_comparator>(t2);
    auto s5 = sort_tuple2<value_comparator>(std::move(t2));
    auto s6 = sort_tuple2<value_comparator>(t3);
    auto s7 = sort_tuple2<value_comparator>(std::move(t3));
    auto s8 = sort_tuple2<value_comparator>(t4);
    auto s9 = sort_tuple2<value_comparator>(std::move(t4));
    auto s10 = sort_tuple2<value_comparator>(t5);
    auto s11 = sort_tuple2<value_comparator>(std::move(t5));
    auto s12 = sort_tuple2<value_comparator>(t6);
    auto s13 = sort_tuple2<value_comparator>(std::move(t6));

    static_assert(std::is_same_v<decltype(s1), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s2), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s3), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s4), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s5), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s6), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s7), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s8), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s9), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s10), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s11), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s12), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s13), std::tuple<test1, test2, test3>>);
}

TEST(SortTuple2, SortTuplesWithAnyElementTypeQualifiers)
{
    test1 v1;
    test2 v2;
    test3 v3;
    std::tuple<test3, test1 &, test2> t1{ v3, v1, v2 };
    std::tuple<test3 &, test1, test2> t2{ v3, v1, v2 };
    std::tuple<test3 &&, const test1, test2> t3{ std::move(v3), v1, v2 };
    std::tuple<const test3, test1, test2 &> t4{ v3, v1, v2 };
    std::tuple<const test3 &, test1 &, const test2 &&> t5{ v3, v1, std::move(v2) };
    std::tuple<const test3 &&, const test1, test2> t6{ std::move(v3), v1, v2 };

    auto s1 = sort_tuple2<value_comparator>(t1);
    auto s2 = sort_tuple2<value_comparator>(std::move(t1));
    auto s3 = sort_tuple2<value_comparator>(t2);
    auto s4 = sort_tuple2<value_comparator>(std::move(t2));
    auto s5 = sort_tuple2<value_comparator>(t3);
    auto s6 = sort_tuple2<value_comparator>(std::move(t3));
    auto s7 = sort_tuple2<value_comparator>(t4);
    auto s8 = sort_tuple2<value_comparator>(std::move(t4));
    auto s9 = sort_tuple2<value_comparator>(t5);
    auto s10 = sort_tuple2<value_comparator>(std::move(t5));
    auto s11 = sort_tuple2<value_comparator>(t6);
    auto s12 = sort_tuple2<value_comparator>(std::move(t6));

    static_assert(std::is_same_v<decltype(s1), std::tuple<test1 &, test2, test3>>);
    static_assert(std::is_same_v<decltype(s2), std::tuple<test1 &, test2, test3 >>);
    static_assert(std::is_same_v<decltype(s3), std::tuple<test1, test2, test3 &>>);
    static_assert(std::is_same_v<decltype(s4), std::tuple<test1, test2, test3 &>>);
    static_assert(std::is_same_v<decltype(s5), std::tuple<const test1, test2, test3 &&>>);
    static_assert(std::is_same_v<decltype(s6), std::tuple<const test1, test2, test3 &&>>);
    static_assert(std::is_same_v<decltype(s7), std::tuple<test1, test2 &, const test3>>);
    static_assert(std::is_same_v<decltype(s8), std::tuple<test1, test2 &, const test3>>);
    static_assert(std::is_same_v<decltype(s9), std::tuple<test1 &, const test2 &&, const test3 &>>);
    static_assert(std::is_same_v<decltype(s10), std::tuple<test1 &, const test2 &&, const test3 &>>);
    static_assert(std::is_same_v<decltype(s11), std::tuple<const test1, test2, const test3 &&>>);
    static_assert(std::is_same_v<decltype(s12), std::tuple<const test1, test2, const test3 &&>>);
}

TEST(SortTuple2, TestValuesInNewTuple)
{
    std::tuple<test3, test1, test2> t{ {5}, {7}, {3} };

    auto s = sort_tuple2<value_comparator>(t);

    EXPECT_EQ(std::get<0>(s).val, 7);
    EXPECT_EQ(std::get<1>(s).val, 3);
    EXPECT_EQ(std::get<2>(s).val, 5);
}

TEST(SortTuple2, ReferenceAsTupleElement)
{
    test1 t1{ 7 };
    test2 t2{ 3 };
    const test3 t3{ 6 };
    const test4 t4{ 10 };

    std::tuple<const test3 &, test1 &, const test4 &&, test2 &&> t
    { t3, t1, std::move(t4), std::move(t2) };

    auto s = sort_tuple2<value_comparator>(t);
    static_assert(std::is_same_v<decltype(s), std::tuple<test1 &, test2 &&,
                  const test3 &, const test4 &&>>);

    t1.val = 15; t2.val = 40;
    EXPECT_EQ(std::get<0>(s).val, 15);
    EXPECT_EQ(std::get<1>(s).val, 40);
    EXPECT_EQ(std::get<2>(s).val, 6);
    EXPECT_EQ(std::get<3>(s).val, 10);

    EXPECT_EQ(&std::get<0>(s), &t1);
    EXPECT_EQ(&std::get<1>(s), &t2);
    EXPECT_EQ(&std::get<2>(s), &t3);
    EXPECT_EQ(&std::get<3>(s), &t4);
}

TEST(SortTuple2, MovesObjects)
{
    std::tuple t{ move_only_test2{4}, move_only_test1{8} };

    auto s = sort_tuple2<value_comparator>(std::move(t));

    EXPECT_EQ(std::get<0>(s).val, 8);
    EXPECT_EQ(std::get<1>(s).val, 4);

    EXPECT_EQ(std::get<0>(t).val, 0);
    EXPECT_EQ(std::get<1>(t).val, 0);
}

TEST(SortTuple2, MovesObjectsOnlyOnce)
{
    move_only_counter::clear();

    std::tuple<move_only_counter, move_only_counter> t;

    auto s = sort_tuple2<value_comparator>(std::move(t));

    EXPECT_EQ(move_only_counter::move_count, 2);
    EXPECT_EQ(move_only_counter::move_assign_count, 0);
}

TEST(SortTuple2, CopiesObjectsOnlyOnce)
{
    copy_only_counter::clear();

    std::tuple<copy_only_counter, copy_only_counter> t;

    auto s = sort_tuple2<value_comparator>(std::move(t));

    EXPECT_EQ(copy_only_counter::copy_count, 2);
    EXPECT_EQ(copy_only_counter::copy_assign_count, 0);
}

TEST(SortTuple2, CheckVariousComparators)
{
    std::tuple<test3, test1, test2> t{};

    auto s1 = sort_tuple2<value_comparator>(t);
    auto s2 = sort_tuple2<value_comparator2>(t);

    static_assert(std::is_same_v<decltype(s1), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s2), std::tuple<test3, test2, test1>>);
}
