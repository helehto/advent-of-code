#include "small_vector.h"
#include <list>
#include <type_traits>
#include <vector>

#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#warnings"
#include <doctest/doctest.h>
#pragma clang diagnostic pop

// Check that constructors and noexcept specifications are sane with POD type.
static_assert(!std::is_default_constructible_v<small_vector_base<int>>);
static_assert(!std::is_copy_constructible_v<small_vector_base<int>>);
static_assert(!std::is_move_constructible_v<small_vector_base<int>>);
static_assert(std::is_nothrow_default_constructible_v<small_vector<int>>);
static_assert(std::is_nothrow_copy_constructible_v<small_vector<int>>);
static_assert(std::is_nothrow_move_constructible_v<small_vector<int>>);
static_assert(std::is_nothrow_destructible_v<small_vector<int>>);
static_assert(std::is_nothrow_swappable_v<small_vector<int>>);

TEST_CASE("small_vector typedefs look sensible")
{
    CHECK(std::same_as<int, small_vector<int, 42>::value_type>);
    CHECK(std::same_as<std::size_t, small_vector<int, 42>::size_type>);
    CHECK(std::same_as<std::ptrdiff_t, small_vector<int, 42>::difference_type>);
    CHECK(std::same_as<int &, small_vector<int, 42>::reference>);
    CHECK(std::same_as<const int &, small_vector<int, 42>::const_reference>);
    CHECK(std::same_as<int *, small_vector<int, 42>::pointer>);
    CHECK(std::same_as<const int *, small_vector<int, 42>::const_pointer>);
    CHECK(std::same_as<std::reverse_iterator<int *>,
                       small_vector<int, 42>::reverse_iterator>);
    CHECK(std::same_as<std::reverse_iterator<const int *>,
                       small_vector<int, 42>::const_reverse_iterator>);
}

TEST_CASE("small_vector iterators are contiguous")
{
    CHECK(std::contiguous_iterator<small_vector<int, 42>::iterator>);
    CHECK(std::contiguous_iterator<small_vector<int, 42>::const_iterator>);
}

template <typename T, std::size_t N>
static void compare_with_std_vector(small_vector<T, N> &v, const std::vector<T> &ref)
{
    SUBCASE("and empty(), size(), and std::distance(begin, end) agree with std::vector")
    {
        CHECK(v.empty() == ref.empty());
        CHECK(v.size() == ref.size());
        CHECK(std::distance(v.begin(), v.end()) == std::distance(ref.begin(), ref.end()));
    }

    SUBCASE("and the values returned from front() and back() agree with std::vector")
    {
        if (!v.empty()) {
            CHECK(v.front() == ref.front());
            CHECK(std::as_const(v).front() == ref.front());
            CHECK(v.back() == ref.back());
            CHECK(std::as_const(v).back() == ref.back());
        }
    }

    SUBCASE("and the values returned from operator[] and at() agree with std::vector")
    {
        for (size_t i = 0; i < ref.size(); ++i) {
            CAPTURE(i);
            CHECK(v[i] == ref[i]);
            CHECK(std::as_const(v)[i] == ref[i]);
            CHECK(v.at(i) == ref.at(i));
            CHECK(std::as_const(v).at(i) == ref.at(i));
        }
    }

    SUBCASE("and the values in [begin(), end()) agree with std::vector")
    {
        for (auto it = v.begin(); T ref_value : ref) {
            CAPTURE(std::distance(v.begin(), it));
            CHECK(*it == ref_value);
            ++it;
        }
    }
}

TEST_CASE("small_vector can be default-initialized")
{
    constexpr size_t InlineCapacity = 8;
    small_vector<int, InlineCapacity> v;

    compare_with_std_vector(v, {});

    SUBCASE("reserving does not increase capacity if it would fit inline")
    {
        v.reserve(5);
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("reserving increases capacity if it is larger than the inline buffer")
    {
        v.reserve(31);
        CHECK(v.capacity() >= 31);
    }

    SUBCASE("adding a single element with push_back works")
    {
        v.push_back(42);
        compare_with_std_vector(v, {42});

        SUBCASE("removing it with pop_back makes the vector empty again")
        {
            v.pop_back();
            compare_with_std_vector(v, {});
        }

        SUBCASE("clearing the vector makes it empty again")
        {
            v.clear();
            compare_with_std_vector(v, {});
        }
    }

    SUBCASE("adding a single element with emplace_back works")
    {
        v.emplace_back(1729);
        compare_with_std_vector(v, {1729});

        SUBCASE("removing it with pop_back makes the vector empty again")
        {
            v.pop_back();
            compare_with_std_vector(v, {});
        }

        SUBCASE("clearing the vector makes it empty again")
        {
            v.clear();
            compare_with_std_vector(v, {});
        }
    }

    SUBCASE("adding fewer elements than the inline capacity works")
    {
        constexpr int NumElements = InlineCapacity - 1;

        std::vector<int> ref;
        for (size_t i = 0; i < NumElements; ++i)
            ref.push_back(i);

        SUBCASE("with push_back")
        {
            for (size_t i = 0; i < NumElements; ++i)
                v.push_back(i);

            compare_with_std_vector(v, ref);

            SUBCASE("clearing the vector makes it empty again")
            {
                v.clear();
                compare_with_std_vector(v, {});
            }
        }

        SUBCASE("with emplace_back")
        {
            for (size_t i = 0; i < NumElements; ++i)
                v.emplace_back(i);

            compare_with_std_vector(v, ref);

            SUBCASE("clearing the vector makes it empty again")
            {
                v.clear();
                compare_with_std_vector(v, {});
            }
        }
    }

    SUBCASE("adding more elements than the inline capacity works")
    {
        constexpr int NumElements = 10 * InlineCapacity;

        std::vector<int> ref;
        for (size_t i = 0; i < NumElements; ++i)
            ref.push_back(i);

        SUBCASE("with push_back")
        {
            for (size_t i = 0; i < NumElements; ++i)
                v.push_back(i);

            compare_with_std_vector(v, ref);

            SUBCASE("clearing the vector makes it empty again")
            {
                v.clear();
                compare_with_std_vector(v, {});
            }
        }

        SUBCASE("with emplace_back")
        {
            for (size_t i = 0; i < NumElements; ++i)
                v.emplace_back(i);

            compare_with_std_vector(v, ref);

            SUBCASE("clearing the vector makes it empty again")
            {
                v.clear();
                compare_with_std_vector(v, {});
            }
        }
    }
}

TEST_CASE("small_vector can be initialized with an initializer list")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("fewer than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v{1, 2, 3, 4, 5};
        compare_with_std_vector(v, {1, 2, 3, 4, 5});
    }

    SUBCASE("equal to the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v{1, 2, 3, 4, 5, 6, 7, 8};
        compare_with_std_vector(v, {1, 2, 3, 4, 5, 6, 7, 8});
    }

    SUBCASE("more than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        compare_with_std_vector(v, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    }
}

TEST_CASE("small_vector can be initialized with a count")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("fewer than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v(InlineCapacity - 1);
        compare_with_std_vector(v, {0, 0, 0, 0, 0, 0, 0});
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("equal to the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v(InlineCapacity);
        compare_with_std_vector(v, {0, 0, 0, 0, 0, 0, 0, 0});
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("more than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v(InlineCapacity + 2);
        compare_with_std_vector(v, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        CHECK(v.capacity() > InlineCapacity);
    }
}

TEST_CASE("small_vector can be initialized with a count and an initial value")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("fewer than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v(InlineCapacity - 1, 42);
        compare_with_std_vector(v, {42, 42, 42, 42, 42, 42, 42});
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("equal to the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v(InlineCapacity, 99);
        compare_with_std_vector(v, {99, 99, 99, 99, 99, 99, 99, 99});
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("more than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v(InlineCapacity + 2, 8);
        compare_with_std_vector(v, {8, 8, 8, 8, 8, 8, 8, 8, 8, 8});
        CHECK(v.capacity() > InlineCapacity);
    }
}

TEST_CASE(
    "small_vector can be initialized from an input iterator range with a sized sentinel")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("fewer than the inline buffer capacity")
    {
        std::array in{0, 1, 2, 3, 4, 5, 6};
        static_assert(std::sized_sentinel_for<decltype(in.end()), decltype(in.begin())>);
        small_vector<int, InlineCapacity> v(in.begin(), in.end());
        compare_with_std_vector(v, std::vector<int>(in.begin(), in.end()));
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("equal to the inline buffer capacity")
    {
        std::array in{0, 1, 2, 3, 4, 5, 6, 7};
        static_assert(std::sized_sentinel_for<decltype(in.end()), decltype(in.begin())>);
        small_vector<int, InlineCapacity> v(in.begin(), in.end());
        compare_with_std_vector(v, std::vector<int>(in.begin(), in.end()));
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("more than the inline buffer capacity")
    {
        std::array in{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        static_assert(std::sized_sentinel_for<decltype(in.end()), decltype(in.begin())>);
        small_vector<int, InlineCapacity> v(in.begin(), in.end());
        compare_with_std_vector(v, std::vector<int>(in.begin(), in.end()));
        CHECK(v.capacity() > InlineCapacity);
    }
}

TEST_CASE("small_vector can be initialized from an input iterator range")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("fewer than the inline buffer capacity")
    {
        std::list<int> in{0, 1, 2, 3, 4, 5, 6};
        small_vector<int, InlineCapacity> v(in.begin(), in.end());
        compare_with_std_vector(v, std::vector<int>(in.begin(), in.end()));
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("equal to the inline buffer capacity")
    {
        std::list<int> in{0, 1, 2, 3, 4, 5, 6, 7};
        small_vector<int, InlineCapacity> v(in.begin(), in.end());
        compare_with_std_vector(v, std::vector<int>(in.begin(), in.end()));
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("more than the inline buffer capacity")
    {
        std::list<int> in{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        small_vector<int, InlineCapacity> v(in.begin(), in.end());
        compare_with_std_vector(v, std::vector<int>(in.begin(), in.end()));
        CHECK(v.capacity() > InlineCapacity);
    }
}

TEST_CASE("small_vector can be copy constructed")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("from an empty small_vector")
    {
        small_vector<int, InlineCapacity> src;
        small_vector<int, InlineCapacity> dst(std::as_const(src));
        compare_with_std_vector(dst, {});
    }

    SUBCASE("from a small_vector with inline elements")
    {
        small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5};
        small_vector<int, InlineCapacity> dst(std::as_const(src));
        compare_with_std_vector(dst, {1, 2, 3, 4, 5});
        CHECK(dst.capacity() == InlineCapacity);
    }

    SUBCASE("from a small_vector with out of line elements")
    {
        small_vector<int, InlineCapacity> src{1,  2,  3,    4,    5,   42,
                                              42, 42, 1729, 1729, 1729};
        small_vector<int, InlineCapacity> dst(std::as_const(src));
        compare_with_std_vector(dst, {1, 2, 3, 4, 5, 42, 42, 42, 1729, 1729, 1729});
        CHECK(dst.capacity() > InlineCapacity);
    }

    SUBCASE("from a small_vector with out of line elements that fit in the inline buffer")
    {
        small_vector<int, InlineCapacity> src(16);
        src.resize(4);
        CHECK(src.capacity() > InlineCapacity);
        small_vector<int, InlineCapacity> dst(std::as_const(src));
        compare_with_std_vector(dst, {0, 0, 0, 0});
        CHECK(dst.capacity() == InlineCapacity);
    }
}

TEST_CASE("small_vector can be copy assigned")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("to an empty small_vector")
    {
        small_vector<int, InlineCapacity> dst;

        SUBCASE("from an empty small_vector")
        {
            small_vector<int, InlineCapacity> src;
            dst = src;
            compare_with_std_vector(dst, {});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with inline elements")
        {
            small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5};
            dst = src;
            compare_with_std_vector(dst, {1, 2, 3, 4, 5});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with out of line elements")
        {
            small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            dst = src;
            compare_with_std_vector(dst, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
            CHECK(dst.capacity() > InlineCapacity);
        }
    }

    SUBCASE("to an small_vector with inline elements")
    {
        small_vector<int, InlineCapacity> dst{1, 2, 3};

        SUBCASE("from an empty small_vector")
        {
            small_vector<int, InlineCapacity> src;
            dst = src;
            compare_with_std_vector(dst, {});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with inline elements")
        {
            small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5};
            dst = src;
            compare_with_std_vector(dst, {1, 2, 3, 4, 5});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with out of line elements")
        {
            small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            dst = src;
            compare_with_std_vector(dst, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
            CHECK(dst.capacity() > InlineCapacity);
        }
    }

    SUBCASE("to an small_vector with out of line elements")
    {
        small_vector<int, InlineCapacity> dst{42, 42, 42, 42, 42, 42, 42, 42, 42, 42};

        SUBCASE("from an empty small_vector")
        {
            small_vector<int, InlineCapacity> src;
            dst = src;
            compare_with_std_vector(dst, {});
            // TODO: The heap buffer capacity should be preserved, not wasted
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with inline elements")
        {
            small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5};
            dst = src;
            compare_with_std_vector(dst, {1, 2, 3, 4, 5});
            // TODO: The heap buffer capacity should be preserved, not wasted
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with out of line elements")
        {
            small_vector<int, InlineCapacity> src{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            dst = src;
            compare_with_std_vector(dst, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
            CHECK(dst.capacity() > InlineCapacity);
        }
    }
}

TEST_CASE("small_vector can be move constructed")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("from an empty small_vector")
    {
        small_vector<int, InlineCapacity> a;
        small_vector<int, InlineCapacity> b(std::move(a));
        compare_with_std_vector(a, {});
        compare_with_std_vector(b, {});
        CHECK(a.capacity() == InlineCapacity);
        CHECK(b.capacity() == InlineCapacity);
    }

    SUBCASE("from a small_vector with inline elements")
    {
        small_vector<int, InlineCapacity> a{1, 2, 3, 4, 5};
        small_vector<int, InlineCapacity> b(std::move(a));
        compare_with_std_vector(a, {});
        compare_with_std_vector(b, {1, 2, 3, 4, 5});
        CHECK(a.capacity() == InlineCapacity);
        CHECK(b.capacity() == InlineCapacity);
    }

    SUBCASE("from a small_vector with out of line elements")
    {
        small_vector<int, InlineCapacity> a{1, 2, 3, 4, 5, 42, 42, 42, 1729, 1729, 1729};
        small_vector<int, InlineCapacity> b(std::move(a));
        compare_with_std_vector(a, {});
        compare_with_std_vector(b, {1, 2, 3, 4, 5, 42, 42, 42, 1729, 1729, 1729});
        CHECK(a.capacity() == InlineCapacity);
        CHECK(b.capacity() > InlineCapacity);
    }
}

TEST_CASE("small_vector can be move assigned")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("to an empty small_vector")
    {
        small_vector<int, InlineCapacity> dst;

        SUBCASE("from an empty small_vector")
        {
            small_vector<int, InlineCapacity> src;
            dst = std::move(src);
            compare_with_std_vector(dst, {});
            compare_with_std_vector(src, {});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with inline elements")
        {
            small_vector<int, InlineCapacity> src(InlineCapacity - 1, 5);
            dst = std::move(src);
            compare_with_std_vector(dst, std::vector<int>(InlineCapacity - 1, 5));
            compare_with_std_vector(src, {});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with out of line elements")
        {
            small_vector<int, InlineCapacity> src(10 * InlineCapacity, 42);
            dst = std::move(src);
            compare_with_std_vector(dst, std::vector<int>(10 * InlineCapacity, 42));
            compare_with_std_vector(src, {});
            CHECK(dst.capacity() > InlineCapacity);
        }
    }

    SUBCASE("to an small_vector with inline elements")
    {
        small_vector<int, InlineCapacity> dst{1, 2, 3};

        SUBCASE("from an empty small_vector")
        {
            small_vector<int, InlineCapacity> src;
            dst = std::move(src);
            compare_with_std_vector(dst, {});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with inline elements")
        {
            small_vector<int, InlineCapacity> src(InlineCapacity - 1, 6);
            dst = std::move(src);
            compare_with_std_vector(dst, std::vector<int>(InlineCapacity - 1, 6));
            compare_with_std_vector(src, {});
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with out of line elements")
        {
            small_vector<int, InlineCapacity> src(10 * InlineCapacity, 1729);
            dst = std::move(src);
            compare_with_std_vector(dst, std::vector<int>(10 * InlineCapacity, 1729));
            compare_with_std_vector(src, {});
            CHECK(dst.capacity() > InlineCapacity);
        }
    }

    SUBCASE("to an small_vector with out of line elements")
    {
        small_vector<int, InlineCapacity> dst(15, 42);

        SUBCASE("from an empty small_vector")
        {
            small_vector<int, InlineCapacity> src;
            dst = std::move(src);
            compare_with_std_vector(dst, {});
            compare_with_std_vector(src, {});
            // TODO: The heap buffer capacity should be preserved, not wasted
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with inline elements")
        {
            small_vector<int, InlineCapacity> src(InlineCapacity - 1, 9);
            dst = std::move(src);
            compare_with_std_vector(dst, std::vector<int>(InlineCapacity - 1, 9));
            compare_with_std_vector(src, {});
            // TODO: The heap buffer capacity should be preserved, not wasted
            CHECK(dst.capacity() == InlineCapacity);
        }

        SUBCASE("from a small_vector with out of line elements")
        {
            small_vector<int, InlineCapacity> src(10 * InlineCapacity, 66);
            dst = std::move(src);
            compare_with_std_vector(dst, std::vector<int>(10 * InlineCapacity, 66));
            compare_with_std_vector(src, {});
            CHECK(dst.capacity() > InlineCapacity);
        }
    }
}

TEST_CASE("small_vector can be filled with an element count and value using assign()")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("fewer than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v;
        v.assign(InlineCapacity - 2, 42);
        compare_with_std_vector(v, std::vector<int>(InlineCapacity - 2, 42));
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("equal to the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v;
        v.assign(InlineCapacity, 11);
        compare_with_std_vector(v, std::vector<int>(InlineCapacity, 11));
        CHECK(v.capacity() == InlineCapacity);
    }

    SUBCASE("more than the inline buffer capacity")
    {
        small_vector<int, InlineCapacity> v;
        v.assign(InlineCapacity + 4, 4);
        compare_with_std_vector(v, std::vector<int>(InlineCapacity + 4, 4));
        CHECK(v.capacity() > InlineCapacity);
    }

    SUBCASE("fewer than the inline buffer capacity after having exceeded it")
    {
        small_vector<int, InlineCapacity> v;
        v.assign(InlineCapacity + 4, 11);
        v.clear();
        v.assign(InlineCapacity - 2, 42);
        compare_with_std_vector(v, std::vector<int>(InlineCapacity - 2, 42));
        CHECK(v.capacity() > InlineCapacity);
    }
}

TEST_CASE("elements can be inserted into small_vector using insert()")
{
    constexpr size_t InlineCapacity = 8;
    small_vector<int, InlineCapacity> a{1, 2, 3, 4, 5, 6};

    SUBCASE("one at a time")
    {
        SUBCASE("at the beginning")
        {
            a.insert(a.begin(), 42);
            a.insert(a.begin(), 43);
            a.insert(a.begin(), 44);
            compare_with_std_vector(a, {44, 43, 42, 1, 2, 3, 4, 5, 6});
        }
        SUBCASE("in the middle")
        {
            a.insert(a.begin() + 3, 42);
            a.insert(a.begin() + 3, 43);
            a.insert(a.begin() + 3, 44);
            compare_with_std_vector(a, {1, 2, 3, 44, 43, 42, 4, 5, 6});
        }
        SUBCASE("at the end")
        {
            a.insert(a.end(), 42);
            a.insert(a.end(), 43);
            a.insert(a.end(), 44);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 42, 43, 44});
        }
    }

    SUBCASE("with an element and a count")
    {
        SUBCASE("at the beginning")
        {
            a.insert(a.begin(), 7, 11);
            compare_with_std_vector(a, {11, 11, 11, 11, 11, 11, 11, 1, 2, 3, 4, 5, 6});
        }
        SUBCASE("in the middle")
        {
            a.insert(a.begin() + 2, 7, 12);
            compare_with_std_vector(a, {1, 2, 12, 12, 12, 12, 12, 12, 12, 3, 4, 5, 6});
        }
        SUBCASE("at the end")
        {
            a.insert(a.end(), 7, 13);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 13, 13, 13, 13, 13, 13, 13});
        }
    }

    SUBCASE("with an initializer list")
    {
        SUBCASE("at the beginning")
        {
            a.insert(a.begin(), {11, 12, 13, 14, 15});
            compare_with_std_vector(a, {11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6});
        }
        SUBCASE("in the middle")
        {
            a.insert(a.begin() + 2, {21, 22, 23, 24, 25});
            compare_with_std_vector(a, {1, 2, 21, 22, 23, 24, 25, 3, 4, 5, 6});
        }
        SUBCASE("at the end")
        {
            a.insert(a.end(), {31, 32, 33, 34, 35});
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 31, 32, 33, 34, 35});
        }
    }
}

TEST_CASE("elements can be erased from a small_vector")
{
    small_vector<int> a{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("from the beginning")
    {
        a.erase(a.begin(), a.begin() + 5);
        compare_with_std_vector(a, {6, 7, 8, 9, 10});
    }
    SUBCASE("from the middle")
    {
        a.erase(a.begin() + 3, a.begin() + 8);
        compare_with_std_vector(a, {1, 2, 3, 9, 10});
    }
    SUBCASE("from the end")
    {
        a.erase(a.end() - 3, a.end());
        compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 7});
    }
}

TEST_CASE("small_vector can be resized with resize() using a default-constructed value")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("from an vector with inline elements")
    {
        small_vector<int, InlineCapacity> a{1, 2, 3, 4, 5};

        SUBCASE("shrinking to a vector with inline elements")
        {
            a.resize(3);
            compare_with_std_vector(a, {1, 2, 3});
            CHECK(a.capacity() == InlineCapacity);
        }

        SUBCASE("growing to a vector with inline elements")
        {
            a.resize(7);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 0, 0});
            CHECK(a.capacity() == InlineCapacity);
        }

        SUBCASE("growing to a vector with out of line elements")
        {
            a.resize(12);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0});
            CHECK(a.capacity() > InlineCapacity);
        }
    }
}

TEST_CASE("small_vector can be resized with resize() using a specified value")
{
    constexpr size_t InlineCapacity = 8;

    SUBCASE("from an vector with inline elements")
    {
        small_vector<int, InlineCapacity> a{1, 2, 3, 4, 5};

        SUBCASE("shrinking to a vector with inline elements")
        {
            a.resize(3, 55);
            compare_with_std_vector(a, {1, 2, 3});
            CHECK(a.capacity() == InlineCapacity);
        }

        SUBCASE("growing to a vector with inline elements")
        {
            a.resize(7, 42);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 42, 42});
            CHECK(a.capacity() == InlineCapacity);
        }

        SUBCASE("growing to a vector with out of line elements")
        {
            a.resize(12, 11);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 11, 11, 11, 11, 11, 11, 11});
            CHECK(a.capacity() > InlineCapacity);
        }
    }
}

TEST_CASE("swapping two small_vectors works")
{
    constexpr int InlineCapacity = 8;

    SUBCASE("with two empty vectors")
    {
        small_vector<int, InlineCapacity> a, b;
        a.swap(b);
        compare_with_std_vector(a, {});
        compare_with_std_vector(b, {});
    }

    SUBCASE("with an empty vector and a vector with elements stored inline")
    {
        small_vector<int, InlineCapacity> a;
        small_vector<int, InlineCapacity> b{1, 2, 3};

        SUBCASE("with the empty vector first")
        {
            a.swap(b);
            compare_with_std_vector(a, {1, 2, 3});
            compare_with_std_vector(b, {});
        }
        SUBCASE("with the non-empty vector first")
        {
            b.swap(a);
            compare_with_std_vector(a, {1, 2, 3});
            compare_with_std_vector(b, {});
        }
    }

    SUBCASE("with an empty vector and a vector with elements stored out of line")
    {
        small_vector<int, InlineCapacity> a;
        small_vector<int, InlineCapacity> b{1, 2, 3, 4, 5, 6, 7, 8, 9};

        SUBCASE("with the empty vector first")
        {
            a.swap(b);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 7, 8, 9});
            compare_with_std_vector(b, {});
        }
        SUBCASE("with the non-empty vector first")
        {
            b.swap(a);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 7, 8, 9});
            compare_with_std_vector(b, {});
        }
    }

    SUBCASE("with a vector with element stored inline and one with elements stored out "
            "of line")
    {
        small_vector<int, InlineCapacity> a{1, 2, 3, 4, 5};
        small_vector<int, InlineCapacity> b{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

        SUBCASE("with the shorter vector first")
        {
            a.swap(b);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
            compare_with_std_vector(b, {1, 2, 3, 4, 5});
        }
        SUBCASE("with the longer vector first")
        {
            b.swap(a);
            compare_with_std_vector(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
            compare_with_std_vector(b, {1, 2, 3, 4, 5});
        }
    }

    SUBCASE("with two vector with elements stored inline")
    {
        small_vector<int, InlineCapacity> a{1, 2, 3};
        small_vector<int, InlineCapacity> b{1, 2, 3, 4, 5};

        SUBCASE("with the shorter vector first")
        {
            a.swap(b);
            compare_with_std_vector(a, {1, 2, 3, 4, 5});
            compare_with_std_vector(b, {1, 2, 3});
        }
        SUBCASE("with the longer vector first")
        {
            b.swap(a);
            compare_with_std_vector(a, {1, 2, 3, 4, 5});
            compare_with_std_vector(b, {1, 2, 3});
        }
    }
}

TEST_CASE("small_vector works with non-default-constructible types")
{
    struct NoDefaultConstructor {
        int a;
        int b;

        NoDefaultConstructor(int a_, int b_) noexcept
            : a(a_)
            , b(b_)
        {
        }
    };

    constexpr int InlineCapacity = 8;
    small_vector<NoDefaultConstructor, InlineCapacity> v;

    SUBCASE("an element can be inserted with push_back(T &&)")
    {
        auto &result = v.push_back(NoDefaultConstructor(42, 1729));
        CHECK(result.a == 42);
        CHECK(result.b == 1729);
    }

    SUBCASE("an element can be inserted with emplace_back()")
    {
        auto &result = v.emplace_back(42, 1729);
        CHECK(result.a == 42);
        CHECK(result.b == 1729);

        SUBCASE("an element can be inserted with emplace()")
        {
            auto it = v.emplace(v.begin(), 123, 456);
            CHECK(&*it == &v[0]);
            CHECK(v[0].a == 123);
            CHECK(v[0].b == 456);
            CHECK(v[1].a == 42);
            CHECK(v[1].b == 1729);
        }
    }
}
