#pragma once

#include "Entity.hpp"
#include "entities/EntityRegistry.hpp"
#include "FileSystemInfo.hpp"
#include "FSCleaner.hpp"

#include <filesystem>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

class YAMLNode;

/**
 * Driver for running commands
 *
 * @ingroup developer
 */
class Driver {
public:
  /**
   * Constructor.
   */
  Driver();

  /**
   * Read in the configuration file.
   */
  void config();

  /**
   * Create a new configuration file.
   */
  void init();

  /**
   * Build documentation.
   */
  void build();

  /**
   * Watch and build documentation on changes.
   */
  void watch();

  /**
   * Output line coverage information.
   */
  void cover();

  /**
   * Clean documentation.
   */
  void clean();

  /**
   * Markdown style.
   */
  std::string style;

  /**
   * Title.
   */
  std::string title;

  /**
   * Description.
   */
  std::string description;

  /**
   * Coverage file.
   */
  std::filesystem::path coverage;

  /**
   * Output directory.
   */
  std::filesystem::path output;

  FSCleaner fscleaner;

private:
  /**
   * Parse files.
   */
  void parse();

  /**
   * Count line coverage.
   */
  void count();

  void aggregate_stats();

  std::optional<std::filesystem::path> get_coverage_path() const;

  std::optional<std::string_view> get_coverage_suffix() const;

  void run_generation();

  /**
   * Recursively read groups from the configuration file.
   */
  static void groups(YAMLNode& parentNode, Entity& parentEntity);

  /**
   * Root entity.
   */
  Entity root;

  std::optional<EntityRegistry> registry;

  /**
   * Configuration file path.
   */
  std::filesystem::path config_file;

  /**
   * Files patterns.
   */
  std::list<std::string> files_patterns;

  /**
   * Macro definitions.
   */
  std::unordered_map<std::string,std::string> defines;

  /**
   * Files.
   */
  std::unordered_set<std::filesystem::path> filenames;

  FileSystemInfo fsinfo;
};
