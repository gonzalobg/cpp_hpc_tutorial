/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 University of Geneva. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <algorithm>
#include <numeric>
#include <vector>
#include <iterator>
#include <iostream>
#include <random>
#if defined(__clang__)
// clang does not support libstdc++ ranges
#include <range/v3/all.hpp>
namespace views = ranges::views;
#elif __cplusplus >= 202002L
#include <ranges>
namespace views = std::views;
#endif

// Initialize vector
void initialize(std::vector<int>& v);

// Select elements and copy them to a new vector
template<class UnaryPredicate>
std::vector<int> select(const std::vector<int>& v, UnaryPredicate pred);


int main(int argc, char* argv[])
{
    // Read CLI arguments, the first argument is the name of the binary:
    if (argc != 2) {
        std::cerr << "ERROR: Missing length argument!" << std::endl;
        return 1;
    }

    // Read length of vector elements
    long long n = std::stoll(argv[1]);

    // Allocate the data vector
    auto v = std::vector<int>(n);

    initialize(v);

    auto predicate = [](int x) { return x % 3 == 0; };
    auto w = select(v, predicate);
    if (!std::all_of(w.begin(), w.end(), predicate) || w.empty()) {
        std::cerr << "ERROR!" << std::endl;
        return 1;
    }
    std::cerr << "OK!" << std::endl;

    std::cout << "w = ";
    std::copy(w.begin(), w.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    return 0;
}

void initialize(std::vector<int>& v)
{
    auto distribution = std::uniform_int_distribution<int> {0, 100};
    auto engine = std::mt19937 {1};
    std::generate(v.begin(), v.end(), [&distribution, &engine]{ return distribution(engine); });
}

// This version of "select" can only run sequentially, because the output
// vector w is built consecutively during the traversal of the input vector v.
template<class UnaryPredicate>
std::vector<int> select(const std::vector<int>& v, UnaryPredicate pred)
{
    std::vector<int> w;
    // NOTE: trying to use back_inserter in parallel introduces a data race!
    std::copy_if(v.begin(), v.end(), std::back_inserter(w), pred);
    return w;
}
