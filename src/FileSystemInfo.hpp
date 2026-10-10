#pragma once

#include "entities/FileEntity.hpp"
#include "entities/FwdRef.hpp"
#include "entities/LineStats.hpp"

#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

class FileSystemInfo {
public:

  FileSystemInfo() = default;

  FileSystemInfo(const FileSystemInfo&) = delete;

  FileSystemInfo& operator=(const FileSystemInfo&) = delete;

  static inline constexpr std::string_view DEF_ROOT_PATH = "./";

  struct DirInfo {
    std::unordered_set<std::string_view> subdirs;
    std::vector<FwdRef<FileEntity>> files;
    LineStats stats;

    DirInfo() = default;
  };

  std::optional<FwdRef<FileEntity>> lookup_file(std::string_view path);

  std::optional<FwdRef<FileEntity>> lookup_file(const std::filesystem::path& path) {
    std::string_view p = path.native();
    return lookup_file(p);
  }

  std::optional<std::reference_wrapper<DirInfo>> lookup_dir(std::string_view path);

  std::optional<std::reference_wrapper<const DirInfo>> lookup_dir(std::string_view path) const;

  void add_file(const std::filesystem::path& path, FwdRef<FileEntity> file);

  void explore() const;

  bool contains(const std::filesystem::path& p) const {
    return files_map.contains(p.native());
  }

private:
  struct string_hash {
      using is_transparent = void; // Enables heterogeneous lookup

      std::size_t operator()(std::string_view sv) const {
          return std::hash<std::string_view>{}(sv);
      }
      std::size_t operator()(const std::string& s) const {
          return std::hash<std::string>{}(s);
      }
  };

  std::unordered_map<std::string, DirInfo, string_hash, std::equal_to<>> dir_info_map;
  std::unordered_map<std::string_view, FwdRef<FileEntity>> files_map;
};

template<bool IS_CONST> struct _DirStackController {

  using _dirinfo_t = std::conditional_t<IS_CONST,
    const FileSystemInfo::DirInfo, FileSystemInfo::DirInfo>;
  using _fsinfo_t = std::conditional_t<IS_CONST,
    const FileSystemInfo, FileSystemInfo>;

  using iter_t = std::conditional_t<IS_CONST,
    std::unordered_set<std::string_view>::const_iterator,
    std::unordered_set<std::string_view>::iterator>;
  using stack_value = std::tuple<std::string_view, _dirinfo_t&, iter_t>;
  using yield_value = std::pair<std::string_view, _dirinfo_t&>;

  _fsinfo_t& fsinfo;
  std::string_view root_dir = FileSystemInfo::DEF_ROOT_PATH;

  stack_value make_init() {
    auto opt = fsinfo.lookup_dir(root_dir);
    if (not opt) {
      throw std::runtime_error(std::format("cannot initialize from {}", root_dir));
    }
    _dirinfo_t& d = opt->get();
    return stack_value{root_dir, d, d.subdirs.begin()};
  }

  yield_value make_yield(const stack_value& top) {
    return yield_value{std::get<0>(top), std::get<1>(top)};
  }

  bool has_next(const stack_value& top) const {
    return std::get<2>(top) != std::get<1>(top).subdirs.end();
  }

  stack_value make_next(stack_value& top) {
    iter_t& it = std::get<2>(top);
    std::string_view next_path = *it;
    ++it;
    auto opt = fsinfo.lookup_dir(next_path);
    if (not opt) {
      throw std::runtime_error(std::format("cannot make next from {}", root_dir));
    }
    _dirinfo_t& d = opt->get();
    return stack_value{next_path, d, d.subdirs.begin()};
  }

  void before_pop(const stack_value&) {}

  void after_pop(const stack_value&) {}
};

using DirStackController = _DirStackController<false>;
using ConstDirStackController = _DirStackController<true>;
