#pragma once

#include <cstddef>

struct LineStats {
  /**
   * Number of lines included in coverage counts.
   */
  std::size_t included_lines = 0;

  /**
   * Number of lines covered in coverage counts.
   */
  std::size_t covered_lines = 0;

  LineStats() = default;

  LineStats& operator+=(const LineStats& ls) {
    included_lines += ls.included_lines;
    covered_lines += ls.covered_lines;
    return *this;
  }
};
