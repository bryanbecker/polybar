#pragma once

#ifndef DEBUG
#error "Not a debug build..."
#endif

#include <chrono>
#include <iostream>

#include "common.hpp"

POLYBAR_NS

namespace debug_util {
  /**
   * Wrapper that starts tracking the time when created
   * and reports the  duration when it goes out of scope
   */
  class scope_timer {
   public:
    using clock_t = std::chrono::high_resolution_clock;
    using duration_t = std::chrono::milliseconds;

    explicit scope_timer() : m_start(clock_t::now()) {}
    ~scope_timer() {
      std::cout << std::chrono::duration_cast<duration_t>(clock_t::now() - m_start).count() << "ms" << std::endl;
    }

   private:
    clock_t::time_point m_start;
  };

  inline unique_ptr<scope_timer> make_scope_timer() {
    return make_unique<scope_timer>();
  }

  template <class T>
  void execution_speed(const T& expr) noexcept {
    auto start = std::chrono::high_resolution_clock::now();
    expr();
    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << "execution speed: " << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count()
              << "ms" << std::endl;
  }

  template <class T>
  void memory_usage(const T& object) noexcept {
    std::cout << "memory usage: " << sizeof(object) << "b" << std::endl;
  }
}

POLYBAR_NS_END
