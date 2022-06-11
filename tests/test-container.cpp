
#include "catch.hpp"
#include "container/circular_queue.hxx"
#include "container/deque.hxx"

TEST_SUITE("misc")
{
    TEST_CASE("Circular queue functions")
    {
        cpph::circular_queue<char> queue{1024};

        for (size_t i = 0; i < 8192; ++i) {
            constexpr char content[] = "hello, world!";

            volatile auto sequence = i * sizeof content;
            queue.enqueue_n(content, sizeof content);

            char buf[sizeof content] = {};
            queue.dequeue_n(sizeof content, buf);

            REQUIRE(std::equal(content, content + sizeof content, buf, buf + sizeof buf));
        }
    }
}
