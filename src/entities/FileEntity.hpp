#pragma once

#include "Documentable.hpp"
#include "EntityFormatter.hpp"
#include "FwdRef.hpp"

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

struct _FileStorage {
  std::string file_path;
};

class FileEntity: protected _FileStorage, public Documentable {
public:
  constexpr FileEntity() = delete;

  constexpr FileEntity(const std::filesystem::path& path, const std::string& content, uint32_t num_lines):
    _FileStorage{path}, Documentable(this->file_path, Doc(), true), content(content), num_lines(num_lines) {}

  constexpr FileEntity(const std::filesystem::path& path, std::string&& content, uint32_t num_lines):
    _FileStorage{path}, Documentable(this->file_path, Doc(), true), content(std::move(content)), num_lines(num_lines) {}

  constexpr std::string_view get_path() const noexcept { return file_path; }

  constexpr const std::string& get_content() const noexcept { return content; }

  constexpr uint32_t get_num_lines() const noexcept { return num_lines; }

private:
  // std::string file_path;
  std::string content;
  uint32_t num_lines;
};

template<> struct std::formatter<FwdRef<FileEntity>, char>: StatelessEntityFormatter {
  template<class FmtContext> constexpr FmtContext::iterator format(const FwdRef<FileEntity>& f, FmtContext& ctx) const {
    return std::format_to(ctx.out(), "FileEntity[{}]", f->get_path());
  }
};
