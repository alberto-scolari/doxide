#pragma once

#include <FileSystemStatsInfo.hpp>

#include <filesystem>
#include <string_view>

class FileSystemDeleter;
class sink_t;

class MdCoverageGenerator {
public:
  /**
    * Constructor.
    *
    * @param output Output directory.
    * @param style Markdown style.
    */
  MdCoverageGenerator(const std::filesystem::path& output, FileSystemDeleter& _fscleaner):
    output(output),
    fscleaner(_fscleaner)
    {}

  /**
    * Generate coverage information.
    *
    * @param root Root entity.
    * @param cov Include code coverage report?
    */
  void generate(const FileSystemStatsInfo& fsinfo);

private:

  struct GenInfo {
    const FileSystemStatsInfo& fsinfo;
    sink_t& sink;
    std::string_view dirpath;
    const FileSystemStatsInfo::DirInfo& dirinfo;
    std::string_view title;
    bool is_not_root;
  };

  void render_dir_coverage(GenInfo& info);

  void sunburst(GenInfo& info);

  void coverage_data(GenInfo& info);

  void coverage_foot(GenInfo& info);

  /**
   * Output directory.
   */
  std::filesystem::path output;

  FileSystemDeleter& fscleaner;
};
