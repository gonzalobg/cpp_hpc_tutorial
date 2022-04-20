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

//! Solves heat equation in 2D, see the README.

#include <algorithm>
#include <cassert>
#include <chrono>
#include <execution>
#include <fstream>
#include <iostream>
#include <vector>

// TODO: will need some new includes here
// For views::cartesian_product we need to use range-v3 until C++23
#include <range/v3/all.hpp>
namespace views = ranges::views;

// Problem parameters
struct parameters {
  double dx, dt;
  long nx, ny, ni;
  int rank = 0, nranks = 1;

  static constexpr double alpha() { return 1.0; } // Thermal diffusivity

  parameters(int argc, char *argv[]) {
    if (argc != 4) {
      std::cerr << "ERROR: incorrect arguments" << std::endl;
      std::cerr << "  " << argv[0] << " <nx> <ny> <ni>" << std::endl;
      std::terminate();
    }
    nx = std::stoll(argv[1]);
    ny = std::stoll(argv[2]);
    ni = std::stoll(argv[3]);
    dx = 1.0 / nx;
    dt = dx * dx / (5. * alpha());
  }

  long nit() { return ni; }
  long nout() { return 1000; }
  long nx_global() { return nx * nranks; }
  long ny_global() { return ny; }
  double gamma() { return alpha() * dt / (dx * dx); }
};

// Index into the memory using row-major order:
long index(long x, long y, parameters p) {
  assert(x >= 0 && x < p.nx);
  assert(y >= 0 && y < p.ny);
  return x * p.ny + y;
};

// Finite-difference stencil
double stencil(double *u_new, double *u_old, long x, long y, parameters p) {
  auto idx = [=](auto x, auto y) { return index(x, y, p); };
  // Apply boundary conditions:
  if (y == 1) {
    u_old[idx(x, y - 1)] = 0;
  }
  if (y == (p.ny - 2)) {
    u_old[idx(x, y + 1)] = 0;
  }
  // These boundary conditions are only impossed by the ranks at the end of the domain:
  if (p.rank == 0 && x == 1) {
    u_old[idx(x - 1, y)] = 1;
  }
  if (p.rank == (p.nranks - 1) && x == p.nx) {
    u_old[idx(x + 1, y)] = 0;
  }

  u_new[idx(x, y)] = (1. - 4. * p.gamma()) * u_old[idx(x, y)] +
                     p.gamma() * (u_old[idx(x + 1, y)] + u_old[idx(x - 1, y)] +
                                  u_old[idx(x, y + 1)] + u_old[idx(x, y - 1)]);

  return u_new[idx(x, y)] * p.dx * p.dx;
}

// 2D grid of indicies
struct grid {
  long x_start, x_end, y_start, y_end;
};

double stencil(double *u_new, double *u_old, grid g, parameters p) {
  double energy = 0.;
  // TODO: implement using parallel algorithms with views::cartesian_product
  return energy;
}

void initialize(double *u_new, double *u_old, long n) {
  std::fill_n(std::execution::par_unseq, u_new, n, 0.);
  std::fill_n(std::execution::par_unseq, u_new, n, 0.);
}

int main(int argc, char *argv[]) {
  // Parse CLI parameters
  parameters p(argc, argv);

  // Allocate memory
  long n = p.nx * p.ny;
  auto u_new = std::vector<double>(n);
  auto u_old = std::vector<double>(n);

  // Initial condition
  initialize(u_new.data(), u_old.data(), n);

  // Time loop
  using clk_t = std::chrono::steady_clock;
  auto start = clk_t::now();

  for (long it = 0; it < p.nit(); ++it) {
    grid g{.x_start = 1, .x_end = p.nx - 1, .y_start = 1, .y_end = p.ny - 1};
    double energy = stencil(u_new.data(), u_old.data(), g, p);
    if (it % p.nout() == 0) {
      std::cerr << "E(t=" << it * p.dt << ") = " << energy << std::endl;
    }
    std::swap(u_new, u_old);
  }

  auto time = std::chrono::duration<double>(clk_t::now() - start).count();
  auto grid_size = static_cast<double>(p.nx * p.ny * sizeof(double) * 2) / 1e9; // GB
  auto memory_bw = grid_size * static_cast<double>(p.nit()) / time;             // GB/s
  if (p.rank == 0) {
    std::cerr << "Domain " << p.nx << "x" << p.ny << " (" << grid_size << " GB): " << memory_bw
              << " GB/s" << std::endl;
  }

  // Write output to file
  std::ios_base::sync_with_stdio(false);
  auto f = std::fstream("output", std::ios::out | std::ios::binary | std::ios::trunc);
  f.write((char *)&p.nx, sizeof(long));
  f.write((char *)&p.ny, sizeof(long));
  double end_time = p.nit() * p.dt;
  f.write((char *)&end_time, sizeof(double));
  f.write((char *)u_new.data(), sizeof(double) * p.nx * p.ny);
  f.flush();
  f.close();

  return 0;
}
