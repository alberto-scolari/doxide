#pragma once

#include "YAMLNode.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>

struct DriverInitInfo {
  std::string style = "";
  std::string title = "";
  std::string description = "";
  std::string coverage = "";
  std::string output = "";

  DriverInitInfo() = default;

  DriverInitInfo(DriverInitInfo&&) = default;
};

class DriverConfigParser {
public:
  DriverConfigParser() = delete;

  DriverConfigParser(const DriverConfigParser&) = delete;

  DriverConfigParser& operator=(const DriverConfigParser&) = delete;

  DriverConfigParser(const std::string_view& _yaml_path);

  std::string_view get_config_file() const noexcept { return yaml_path; }

  const YAMLNode& get_root() const noexcept { return root; }

  void parse_init_info(DriverInitInfo& info) const;

  std::unordered_map<std::string, std::string> get_defines() const;

  std::pair<std::unordered_set<std::filesystem::path>, std::vector<std::string>>
    get_filenames_and_patterns() const;

private:
  std::string yaml_path;
  YAMLNode root;
};
