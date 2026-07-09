#pragma once

#include <ostream>
#include <format>
#include <iterator>
#include <iostream>
#include <utility>

class sink_t {
public:
  using output_iter_t = std::ostream_iterator<char>;
  sink_t(std::ostream& s): _it(s) {}

  sink_t(): sink_t(std::cout) {}

  template<class... Args> output_iter_t print(std::format_string<Args...> fmt, Args&&... args ) {
    return std::format_to(_it, fmt, std::forward<Args>(args)...);
  }

  template<class... Args> output_iter_t println(std::format_string<Args...> fmt, Args&&... args ) {
    output_iter_t it = this->print(fmt, std::forward<Args>(args)...);
    it = '\n';
    return it;
  }

  inline output_iter_t println() {
    _it = '\n';
    return _it;
  }

private:
  output_iter_t _it;
};
