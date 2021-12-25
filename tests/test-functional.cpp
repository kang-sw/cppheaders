#include <chrono>
#include <memory>

#include <functional.hxx>
#include "../third/doctest.h"

TEST_SUITE("functional")
{
    TEST_CASE("compilation")
    {
        cpph::function<int(void)> s;
        s = []
        {
            return 1;
        };

        CHECK(s() == 1);
        CHECK(s.is_sbo());

        std::unique_ptr<int> ptr{new int{3}};
        s = [ptr = std::move(ptr)]
        {
            return *ptr;
        };

        CHECK(s() == 3);
        CHECK(s.is_sbo());

        auto d = std::move(s);

        REQUIRE_THROWS(s());
        CHECK(d() == 3);
        CHECK(d.is_sbo());

        s = std::function<int(void)>{
                []
                {
                    return 444;
                }};

        CHECK(s.is_sbo());
        CHECK(s() == 444);
    }

    struct lambda_t
    {
        std::array<char, 40> bf;

        void operator()()
        {
            volatile int s = 0;
            s              = 1;
            s              = 0;
        }
    };

    TEST_CASE("benchmark")
    {
        return;

        cpph::function<void()> test1 = lambda_t{};
        std::function<void()> test2  = lambda_t{};

        using clk = std::chrono::system_clock;

        auto n1 = clk::now();
        for (auto i = 0; i < 400'000; ++i)
        {
            test1 = lambda_t{};
            test1();
        }
        auto n2 = clk::now();

        for (auto i = 0; i < 400'000; ++i)
        {
            test2 = lambda_t{};
            test2();
        }
        auto n3 = clk::now();

        auto t_1 = n2 - n1;
        auto t_2 = n2 - n1;

        MESSAGE("..."
                << '\n'
                << "CPPH FUNCTION: " << std::chrono::duration<double>(t_1).count() << '\n'
                << "STD  FUNCTION: " << std::chrono::duration<double>(t_2).count());
    }

    TEST_CASE("release")
    {
        auto s               = std::make_shared<int>();
        std::weak_ptr<int> w = s;

        cpph::function<int()> f{
                [s = std::move(s)]
                {
                    return s.use_count();
                }};

        REQUIRE(f() == 1);

        SUBCASE("iter 0")
        {
            f = {};

            CHECK(w.expired());
        }

        SUBCASE("iter 1")
        {
            auto g = std::move(f);
            REQUIRE_THROWS(f());

            CHECK(g() == 1);
            g = {};

            CHECK(w.expired());
        }
    }
}

template <size_t N>
class la;
