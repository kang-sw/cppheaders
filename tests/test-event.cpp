#include "catch.hpp"
#include "cpph/utility/event.hxx"
using namespace cpph;

TEST_SUITE("event.hxx")
{
    TEST_CASE("SingleThread: Add")
    {
        event_st<int, int&, const int&> event;
        event += [](auto a, auto& b, auto c) { ++a, ++b, ++c; };
        event += [](auto a, auto& b, auto c) { ++a, ++b, ++c; };
        event += [](auto a, auto& b, auto c) { ++a, ++b, ++c; };

        int a = 10, b = 10, c = 10;
        event.invoke(a, b, c);

        REQUIRE(a == 10), REQUIRE(b == 13), REQUIRE(c == 10);
    }

    TEST_CASE("SingleThread: Remove")
    {
        event_st<int&> event;
        auto h1 = event.add([](auto& a) { a += 1; });
        auto h2 = event.add([](auto& a) { a += 10; });
        auto h3 = event.add([](auto& a) { a += 100; });

        int a = 0;
        SUBCASE("Remove h1")
        {
            event.remove(move(h1));
            event.invoke(a);
            REQUIRE(a == 110);
        }
        SUBCASE("Remove h3, h2")
        {
            event.remove(move(h3));
            event.remove(move(h2));
            event.invoke(a);
            REQUIRE(a == 1);
        }
    }

    TEST_CASE("SingleThread: Remove With Proxy API")
    {
        event_st<int&> event;
        decltype(event)::handle h1, h2, h3;
        event() << &h1 << [](auto& a) { a += 1; };
        event() << &h2 << [](auto& a) { a += 10; }
                << [](auto& a) { a += 10; }
                << [](auto& a) { a -= 10; };
        event() << &h3 << [](auto& a) { a += 100; };

        int a = 0;
        SUBCASE("Remove h1")
        {
            event.remove(move(h1));
            event.invoke(a);
            REQUIRE(a == 110);
        }
        SUBCASE("Remove h3, h2")
        {
            event.remove(move(h3));
            event.remove(move(h2));
            event.invoke(a);
            REQUIRE(a == 1);
        }
        SUBCASE("Time Taken")
        {
            for (int i = 0; i < 10'000'000; ++i) {
                event.invoke(i);
            }
        }
    }

    TEST_CASE("SingleThread: Invocation Order") {}
    TEST_CASE("SingleThread: ") {}
}
