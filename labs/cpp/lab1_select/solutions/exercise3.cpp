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
#include <chrono>
#include <numeric>
#include <vector>
#include <iterator>
#include <iostream>
#include <random>
#include <ranges>
#include <execution>

// Select elements and copy them to a new vector
template<class UnaryPredicate>
void select(const std::vector<int>& v, UnaryPredicate pred, 
            std::vector<size_t>& index, std::vector<int>& w)
{
    // transform_inclusive_scan first filters the data with a "transform" operation
    // and then computes an inclusive cumulative operation (here a sum).
    index.resize(v.size());
    std::transform_inclusive_scan(std::execution::par, v.begin(), v.end(), index.begin(), std::plus<size_t>{},
                                  [pred](int x) { return pred(x) ? 1 : 0; });

    size_t numElem = index.empty() ? 0 : index.back();
    w.resize(numElem);

    auto ints = std::views::iota(0, (int)v.size());
    std::for_each(std::execution::par, ints.begin(), ints.end(),
        [pred, v=v.data(), w=w.data(), index=index.data()](int i) {
            if (pred(v[i])) w[index[i] - 1] = v[i];
    });
}

// Initialize vector
void initialize(std::vector<int>& v);

// Benchmarks the implementation
template <typename Predicate>
void bench(std::vector<int>& v, Predicate&& predicate, std::vector<size_t>& index, std::vector<int>& w);

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
    std::vector<size_t> index;
    std::vector<int> w;                       
    select(v, predicate, index, w);
    if (!std::all_of(w.begin(), w.end(), predicate) || w.empty()) {
        std::cerr << "ERROR!" << std::endl;
        return 1;
    }
    std::cerr << "OK!" << std::endl;

    if (n < 40) {
        std::cout << "w = ";
        std::copy(w.begin(), w.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
    }
    
    bench(v, predicate, index, w);

    return 0;
}

void initialize(std::vector<int>& v)
{
    auto distribution = std::uniform_int_distribution<int> {0, 100};
    auto engine = std::mt19937 {1};
    std::generate(v.begin(), v.end(), [&distribution, &engine]{ return distribution(engine); });
}

template <typename Predicate>
void bench(std::vector<int>& v, Predicate&& predicate, std::vector<size_t>& index, std::vector<int>& w) {
  // Measure bandwidth in [GB/s]
  using clk_t = std::chrono::steady_clock;
  select(v, predicate, index, w);
  auto start = clk_t::now();
  int nit = 100;
  for (int it = 0; it < nit; ++it) {
    select(v, predicate, index, w);
  }
  auto seconds = std::chrono::duration<double>(clk_t::now() - start).count(); // Duration in [s]
  // Amount of bytes transferred from/to chip.
  // v is read (int), written to index (size_t), then v and index are read (int, size_t), and written to w (int):
  auto gigabytes = (3. * sizeof(int) + 2. * sizeof(size_t)) * (double)v.size() * (double)nit * 1.e-9; // GB
  std::cerr << "Bandwidth [GB/s]: " << (gigabytes / seconds) << std::endl;
}