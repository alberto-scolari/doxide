#include "FileSystemInfo.hpp"

#include <stdexcept>

#include <iostream>

std::optional<FwdRef<FileEntity>> FileSystemInfo::lookup_file(std::string_view path) {
  auto it = files_map.find(path);
  if (it == files_map.end()) {
    return {};
  }
  return it->second;
}

std::optional<std::reference_wrapper<FileSystemInfo::DirInfo>> FileSystemInfo::lookup_dir(std::string_view path) {
  auto it = dir_info_map.find(path);
  if (it == dir_info_map.end()) {
    return {};
  }
  return it->second;
}

std::optional<std::reference_wrapper<const FileSystemInfo::DirInfo>> FileSystemInfo::lookup_dir(std::string_view path) const {
  auto it = dir_info_map.find(path);
  if (it == dir_info_map.cend()) {
    return {};
  }
  return it->second;
}

void FileSystemInfo::add_file(const std::filesystem::path& path, FwdRef<FileEntity> file) {
  namespace fs = std::filesystem;

  if (path.empty() or not path.has_filename()) {
    throw std::runtime_error(std::format("empty file path or no filename"));
  }

  fs::path normalized = path.lexically_normal();
  auto [_, newly_created] = files_map.try_emplace(file->get_name(), file);
  if (not newly_created) {
    throw std::runtime_error(std::format("file '{}' was already added", normalized.native()));
  }

  auto get_or_create_path = [&](std::string_view path) {
    auto it = dir_info_map.find(path);
    if (it == dir_info_map.end()) {
      auto [pit, _] = dir_info_map.try_emplace(std::string(path));
      it = pit;
    }
    return it;
  };

  fs::path parent = normalized.parent_path();
  std::string_view parent_view = not parent.empty() ? parent.native() : DEF_ROOT_PATH;
  auto it = get_or_create_path(parent_view);
  it->second.files.push_back(file);
  if (parent.empty()) {
    return;
  }

  parent = parent.parent_path();
  while(not parent.empty()) {
    auto pit = get_or_create_path(parent.native());
    pit->second.subdirs.insert(it->first);
    it = pit;
    parent = parent.parent_path();
  }

  auto pit = get_or_create_path(DEF_ROOT_PATH);
  pit->second.subdirs.insert(it->first);
}

void FileSystemInfo::explore() const {
  std::cout << "DIRS\n";
  for(const auto& [k, v]: dir_info_map) {
    std::cout << "----- " << k << ":\n";
    for(const auto& f: v.subdirs) {
      std::cout << "  dir: " << f << "\n";
    }
    for(const auto& f: v.files) {
      std::cout << "  file: " << f->get_name() << "\n";
    }
    std::cout << "\n";
  }
  std::cout << "FILES\n";
  for(const auto& [k, _]: files_map) {
    std::cout << "----- " << k << "\n";
  }
}
