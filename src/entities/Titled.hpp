#pragma once

#include <string>
#include <string_view>

class Titled {
public:
  constexpr Titled(std::string_view name, std::string_view _title): stored_name(name), title(_title) {}

  constexpr std::string_view get_title() const noexcept { return title; }

protected:
  std::string stored_name;
  std::string title;
};
