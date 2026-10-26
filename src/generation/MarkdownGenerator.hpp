#pragma once

#include <FileSystemInfo.hpp>
#include <FSCleaner.hpp>
#include <Sink.hpp>

#include <filesystem>
#include <optional>
#include <string_view>

class EntityRegistry;
class RefVariant;

class MarkdownGenerator {
public:
  /**
    * Constructor.
    *
    * @param output Output directory.
    * @param style Markdown style.
    */
  MarkdownGenerator(const std::filesystem::path& output, FSCleaner& _fscleaner):
    output(output),
    fscleaner(_fscleaner)
    {}

  /**
    * Generate documentation.
    *
    * @param root Root entity.
    * @param cov Include code coverage report?
    */
  void generate(const EntityRegistry& registry, const std::optional<std::string_view>& coverage_directory);

private:
  void render_entity(const EntityRegistry& registry, const std::filesystem::path& output, const RefVariant& root, const std::string_view& sanitized_name, const std::optional<std::string_view>& coverage_directory);

  /**
   * Output directory.
   */
  std::filesystem::path output;

  FSCleaner& fscleaner;

  /**
   * Markdown style.
   */
  std::string_view style;
};
