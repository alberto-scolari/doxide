#pragma once

#include <string>
#include <string_view>

class DirEntity {
public:
  constexpr DirEntity(std::string_view n): name(n) {}

  constexpr std::string_view get_name() const noexcept { return name; }

private:
  std::string name;
};
