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

#include <type_traits>

// Function template test
template <typename T, typename = std::enable_if_t<std::is_same_v<T, void>>>
auto test_a() -> decltype((std::enable_if_t<std::is_same_v<T, void>>*){}, nullptr)
{
    return nullptr;
}

// Utilize test_a in template function
template <typename Other>
auto test_b()
{
    return test_a<Other>();
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, int>>>
auto test_a() -> decltype(nullptr)
{
    return nullptr;
}

void main_1()
{
    test_b<void>();
    // test_b<int>(); error - int specialization is invisible in test_b
}

template <typename T, typename = void>
struct test_strt
{
};

template <typename T>
auto test_strt_a()
{
    return test_strt<T>{}();
}

template <typename T>
struct test_strt<T, std::enable_if_t<std::is_same_v<T, int>>>
{
    auto operator()()
    {
        return int{};
    }
};

void main_2()
{
    static_assert(std::is_same_v<decltype(test_strt<int>{}()), int>);
    static_assert(std::is_same_v<decltype(test_strt_a<int>()), int>);
}

int main()
{
    main_1();
    main_2();
}
