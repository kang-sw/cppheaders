/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#include "catch.hpp"
#include "thread/event_queue.hxx"
#include "utility/counter.hxx"

static std::atomic_size_t invoked = 0;
static std::atomic_size_t destructed = 0;

template <typename Ty>
struct test_invocable_t {
    bool once_flag;
    Ty data;

    void operator()()
    {
        invoked.fetch_add(1, std::memory_order_relaxed);
    }

    ~test_invocable_t()
    {
        if (once_flag)
            destructed.fetch_add(1, std::memory_order_relaxed);
    }

    test_invocable_t() noexcept
            : once_flag(true),
              data()
    {
    }

    explicit test_invocable_t(Ty&& arg)
            : once_flag(true),
              data(std::forward<Ty>(arg))
    {
    }

    test_invocable_t(test_invocable_t&& other) noexcept
            : data(std::move(other.data)),
              once_flag(std::exchange(other.once_flag, false))
    {
    }

    test_invocable_t& operator=(test_invocable_t&& other) noexcept
    {
        data = std::move(other.data);
        once_flag = std::exchange(other.once_flag, false);
    }
};

TEST_SUITE("thread")
{
    TEST_CASE("Test message procedure features")
    {
        cpph::event_queue mproc{10 << 20};
        using invocable_t = test_invocable_t<std::array<int, 1024>>;
        int N = 10'000;

        invoked = destructed = 0;

        SUBCASE("Execution count equality")
        {
            for (int i = 0; i < N; ++i)
                mproc.post(invocable_t{});

            SUBCASE("ST")
            {
                mproc.flush();
            }

            SUBCASE("MT")
            {
                std::vector<std::thread> pool;
                for (auto i : cpph::count(std::thread::hardware_concurrency()))
                    pool.emplace_back(std::thread{&decltype(mproc)::exec, &mproc});

                while (destructed != N)
                    std::this_thread::yield();

                mproc.stop();
                for (auto& th : pool) { th.join(); }
            }

            REQUIRE(invoked == N);
            REQUIRE(destructed == N);
        }

        SUBCASE("Provider")
        {
            std::vector<std::thread> pool;

            SUBCASE("SPSC")
            {
                pool.emplace_back(std::thread{&decltype(mproc)::exec, &mproc});

                for (int i = 0; i < N; ++i)
                    mproc.post(invocable_t{});

                while (destructed != N)
                    std::this_thread::yield();
            }

            SUBCASE("SPMC")
            {
                for (auto i : cpph::count(std::thread::hardware_concurrency()))
                    pool.emplace_back(std::thread{&decltype(mproc)::exec, &mproc});

                for (int i = 0; i < N; ++i)
                    mproc.post(invocable_t{});

                while (destructed != N)
                    std::this_thread::yield();
            }

            SUBCASE("MPSC")
            {
                pool.emplace_back(std::thread{&decltype(mproc)::exec, &mproc});

                N /= std::thread::hardware_concurrency();

                for (auto i : cpph::count(std::thread::hardware_concurrency()))
                    pool.emplace_back([&mproc, N] {
                        for (auto k : cpph::count(N)) {
                            mproc.post(invocable_t{});
                        }
                    });

                N *= std::thread::hardware_concurrency();

                while (destructed != N)
                    std::this_thread::yield();
            }

            SUBCASE("MPMC")
            {
                for (auto i : cpph::count(std::thread::hardware_concurrency()))
                    pool.emplace_back(std::thread{&decltype(mproc)::exec, &mproc});

                N /= std::thread::hardware_concurrency();

                for (auto i : cpph::count(std::thread::hardware_concurrency()))
                    pool.emplace_back([&mproc, N] {
                        for (auto k : cpph::count(N)) {
                            mproc.post(invocable_t{});
                        }
                    });

                N *= std::thread::hardware_concurrency();

                while (destructed != N)
                    std::this_thread::yield();
            }

            // Join all
            mproc.stop();
            for (auto& th : pool) { th.join(); }

            REQUIRE(invoked == N);
            REQUIRE(destructed == N);
        }
    }
}
