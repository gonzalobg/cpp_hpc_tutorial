/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
// DONE: add C++ standard library includes as necessary
#if defined(__INTEL_LLVM_COMPILER)
#include <oneapi/dpl/execution>
#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/ranges>
#else
#include <algorithm>
#include <execution>
#endif

#if defined(__clang__) && !defined(__INTEL_LLVM_COMPILER)
// clang does not support libstdc++ ranges
#include <range/v3/all.hpp>
namespace views = ranges::views;
#elif __cplusplus >= 202002L && !defined(__INTEL_LLVM_COMPILER)
#include <ranges>
namespace views = std::views;
namespace ranges = std::ranges;
#endif

#if defined(__INTEL_LLVM_COMPILER)
namespace execution = oneapi::dpl::execution;
#else
namespace execution = std::execution;
#endif

// Initialize vectors
void initialize(std::vector<double> &x, std::vector<double> &y);

// DAXPY
void daxpy(double a, std::vector<double> const &x, std::vector<double> &y);

// Check solution
bool check(double a, std::vector<double> const &y);

int main(int argc, char *argv[]) {
  // Read CLI arguments, the first argument is the name of the binary:
  if (argc != 2) {
    std::cerr << "ERROR: Missing length argument!" << std::endl;
    return 1;
  }

  // Read length of vector elements
  long long n = std::stoll(argv[1]);

  // Allocate the vector
  std::vector<double> x(n, 0.), y(n, 0.);
  double a = 2.0;

  initialize(x, y);

  daxpy(a, x, y);

  if (!check(a, y)) {
    std::cerr << "ERROR!" << std::endl;
    return 1;
  }

  std::cerr << "OK!" << std::endl;

  // Measure bandwidth in [GB/s]
  using clk_t = std::chrono::steady_clock;
  daxpy(a, x, y);
  auto start = clk_t::now();
  int nit = 100;
  for (int it = 0; it < nit; ++it) {
    daxpy(a, x, y);
  }
  auto seconds = std::chrono::duration<double>(clk_t::now() - start).count(); // Duration in [s]
  // Amount of bytes transferred from/to chip.
  // x is read, y is read and written:
  auto gigabytes =
      static_cast<double>((x.size() + 2 * y.size()) * sizeof(double) * nit) / 1.e9; // GB
  std::cerr << "Bandwidth [GB/s]: " << (gigabytes / seconds) << std::endl;

  return 0;
}

bool check(double a, std::vector<double> const &y) {
  double tolerance = 2. * std::numeric_limits<double>::epsilon();
  for (std::size_t i = 0; i < y.size(); ++i) {
    double should = a * i + 2.;
    if (std::abs(y[i] - should) > tolerance)
      return false;
  }
  return true;
}

void initialize(std::vector<double> &x, std::vector<double> &y) {
  assert(x.size() == y.size());
  // DONE: Implement using the C++ parallel Standard Template Library algorithms

#if __cplusplus >= 202002L
  // In C++20 or newer we can just use ranges:
  auto ints = views::iota(0, (int)x.size());
  // Note: there is no <ranges> version of the parallel algorithms in standard C++ yet
  // so we need to use the iterator-based versions. Notice that ranges provide iterators:
  std::transform(execution::par_unseq, ints.begin(), ints.end(), x.begin(),
                 [](auto v) { return (double)v; });
  std::fill(execution::par_unseq, y.begin(), y.end(), 2.0);
#else
  // In C++17 we can either use range-v3, or compute indices from the pointers:
  std::transform(execution::par_unseq, x.begin(), x.end(), x.begin(),
                 [x = x.data()](double const &v) {
                   int index = &v - x; // obtain index of element
                   return (double)index;
                 });
  std::fill(execution::par_unseq, y.begin(), y.end(), 2.0);
#endif
}

void daxpy(double a, std::vector<double> const &x, std::vector<double> &y) {
  assert(x.size() == y.size());
  // DONE: Implement using the C++ parallel Standard Template Library algorithms yet
#if __cplusplus >= 202002L
  // In C++20 or newer we can just use ranges:

  auto ints = views::iota(0, (int)x.size());
  // note: when using parallel algorithms, prefer to capture by value to improve the performance
  // of offloading these to devices
  std::for_each(execution::par_unseq, ints.begin(), ints.end(),
                [=, y = y.data(), x = x.data()](auto i) { y[i] += a * x[i]; });
#else
#if defined(USE_TRANSFORM2)
  // In C++17, we can either use the two range form of transform
  std::transform(execution::par_unseq, x.begin(), x.end(), y.begin(), y.begin(),
                 [a](double x, double y) { return a * x + y; });
#else
  // or use range-v3, or compute indices from the pointers:
  // note: when using parallel algorithms, prefer to capture by value to improve the performance
  // of offloading these to devices
  std::transform(execution::par_unseq, x.begin(), x.end(), y.begin(),
                 [&, a, y = y.data(), x = x.data()](double const &v) {
                   int index = &v - x;
                   return a * x[index] + y[index];
                 });
#endif // TRANSFORM2
#endif
}
