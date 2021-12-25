#include <vector>

#include <memory/queue_allocator.hxx>

#include "../third/doctest.h"

TEST_SUITE("queue_allocator")
{
    TEST_CASE("crash test")
    {
        enum
        {
            BUFLEN = 1024
        };

        cpph::queue_buffer buffer{BUFLEN};
        REQUIRE(buffer.capacity() == BUFLEN);
        REQUIRE_THROWS(buffer.allocate(0));
        REQUIRE_THROWS(buffer.allocate(BUFLEN));

        {
            void* ptr;
            REQUIRE_NOTHROW(ptr = buffer.allocate(BUFLEN - 8));
            REQUIRE(buffer.size() == 1);
            buffer.deallcoate(ptr);
            REQUIRE(buffer.empty());
        }

        int inserter = 0;
        int reader   = 0;

        // allocate half
        for (size_t i = 0; i < BUFLEN / 8 / 2; ++i)
        {
            INFO(""
                 << "buffer size: " << buffer.size() << "\n"
                 << "iteration: " << i << "\n"
                 << "-----------------------------------");
            int* ptr;
            REQUIRE_NOTHROW(ptr = (int*)buffer.allocate(8));
            *ptr = inserter++;
            REQUIRE(buffer.size() == i + 1);
        }

        REQUIRE_THROWS(buffer.allocate(1));

        // deallocate half of half
        auto first_size = buffer.size();
        for (size_t i = 0; i < BUFLEN / 8 / 2 / 2; ++i)
        {
            CHECK(*(int*)buffer.front() == reader++);

            buffer.deallcoate(buffer.front());
            CHECK(buffer.size() == first_size - i - 1);
        }

        // again, insert.
        first_size = buffer.size();
        for (size_t i = 0; i < BUFLEN / 8 / 2 / 2; ++i)
        {
            int* ptr = (int*)buffer.allocate(8);
            *ptr     = inserter++;
            REQUIRE(buffer.size() == first_size + i + 1);
        }

        first_size = buffer.size();
        for (size_t i = 0; i < BUFLEN / 8 / 2; ++i)
        {
            CHECK(*(int*)buffer.front() == reader++);

            buffer.deallcoate(buffer.front());
            REQUIRE(buffer.size() == first_size - i - 1);
        }

        REQUIRE(buffer.empty());
    }

    TEST_CASE("buffer deferred dealloc")
    {
    }

    TEST_CASE("typed allocator")
    {
        cpph::queue_allocator alloc{1024};
        {
            auto p = alloc.construct<int>();

            auto ptr  = alloc.checkout<int>();
            auto ptr1 = alloc.checkout<int[]>(131);

            REQUIRE_NOTHROW(ptr1.at(130));
            REQUIRE_THROWS(ptr1.at(131));

            alloc.destruct(p);
        }
    }
}
