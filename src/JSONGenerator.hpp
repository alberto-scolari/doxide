#pragma once

class FileSystemInfo;

/**
 * JSON generator for coverage data following gcov/gcovr schema.
 *
 * @ingroup developer
 */
class JSONGenerator {
public:
  /**
   * Generate coverage data.
   *
   * @param root Root entity.
   */
  void generate(const FileSystemInfo& registry);
};
