#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <filesystem>

class CodeInfo {
protected:
  std::string name;
  std::string decl;
  std::filesystem::path path;
  uint32_t start_line;
  uint32_t end_line;
  std::string_view docs;
  bool visible;

  CodeInfo(const std::string& name, const std::string& decl, const std::filesystem::path& path, uint32_t start_line, uint32_t end_line, std::string_view docs, bool visible);
};
