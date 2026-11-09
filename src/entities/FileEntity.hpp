#pragma once

#include "Documentable.hpp"
#include "FileUtils.hpp"
#include "EntityFormatter.hpp"
#include "FwdRef.hpp"
#include "LineStats.hpp"

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

struct FileStorage {
  path_str file_path;
  path_str_view filename;

  constexpr FileStorage(const std::filesystem::path& path): file_path(path.native()),
    filename(file_utils::get_filename(file_path)) {}
};

class FileEntity: protected FileStorage, public Documentable, public LineStats {
public:
  constexpr FileEntity() = delete;

  constexpr FileEntity(const std::filesystem::path& path, const std::string& content, uint32_t num_lines):
    FileStorage{path}, Documentable(this->file_path, Doc(), true), content(content), num_lines(num_lines) {}

  constexpr FileEntity(const std::filesystem::path& path, std::string&& content, uint32_t num_lines):
    FileStorage{path}, Documentable(this->file_path, Doc(), true), content(std::move(content)), num_lines(num_lines) {}

  constexpr path_str_view get_path() const noexcept { return file_path; }

  constexpr path_str_view get_filename() const noexcept { return filename; }

  constexpr const std::string_view get_content() const noexcept { return content; }

  constexpr uint32_t get_num_lines() const noexcept { return num_lines; }

  constexpr auto& get_line_counts() noexcept {
    if (line_counts.size() < get_num_lines()) {
      line_counts.resize(get_num_lines(), -1);
    }
    return line_counts;
  }

  constexpr const auto& get_line_counts() const noexcept { return line_counts; }

  constexpr auto& get_included_lines() noexcept { return this->included_lines; }

  constexpr auto get_included_lines() const noexcept { return this->included_lines; }

  constexpr void increment_covered_lines() noexcept { ++this->covered_lines; }

  constexpr auto get_covered_lines() const noexcept { return this->covered_lines; }

private:
  std::string content;
  uint32_t num_lines;

  /**
   * Execution counts for lines. -1 for a line indicates that
   * it is excluded.
   */
  std::vector<int> line_counts;
};

template<> struct std::formatter<FwdRef<FileEntity>, char>: StatelessEntityFormatter {
  template<class FmtContext> constexpr FmtContext::iterator format(const FwdRef<FileEntity>& f, FmtContext& ctx) const {
    return std::format_to(ctx.out(), "FileEntity[{}]", f->get_path());
  }
};
