#include "JSONCounter.hpp"

#include "Log.hpp"
#include "YAMLNode.hpp"
#include "YAMLParser.hpp"
#include "FileSystemInfo.hpp"

#include <stddef.h>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

void JSONCounter::count(const std::filesystem::path& coverage, FileSystemInfo& fsinfo) {
  YAMLParser parser;
  YAMLNode node = parser.parse(coverage);
  if (!node.isSequence("files")) {
    warn("missing 'files' key in " << coverage);
  } else for (auto file : node.sequence("files")) {
    if (!file->isValue("file")) {
      warn("missing 'file' key in 'files' element in " << file);
    } else if (!file->isSequence("lines")) {
      warn("missing 'lines' key in 'files' element in " << file);
    } else {
      auto lines = file->sequence("lines");
      std::filesystem::path path = file->value("file");
      if (path.is_absolute()) {
        /* make the path relative to the current working directory */
        path = std::filesystem::relative(path);
      }
      path = path.lexically_normal();
      std::optional<FwdRef<FileEntity>> file = fsinfo.lookup_file(path);
      if (file) {  // otherwise not included
        /* update line coverage for this file */
        FwdRef<FileEntity> f = *file;
        size_t nlines = f->get_line_counts().size();
        for (auto line : lines) {
          if (!line->has("line_number")) {
            warn("missing 'line_number' key in 'lines' element in " << f->get_name());
          } else if (!line->has("count")) {
            warn("missing 'count' key in 'lines' element in " << f->get_name());
          } else {
            int line_number = std::stoi(line->value("line_number")) - 1;
            int count = std::stoi(line->value("count"));
            if (line_number < 0 || size_t(line_number) >= nlines) {
              warn("in " << path << ":" << line_number <<
                  " does not exist; ignoring, are source and coverage" <<
                  " files in sync?");
            } else if (count > 0) {
              int& line_count = f->get_line_counts()[line_number];
              if (line_count == 0) {
                /* line is included in report and first time seeing it
                 * covered, update aggregate counts for whole path */
                f->increment_covered_lines();
              }
              if (line_count >= 0) {
                /* line is included in report, update count */
                line_count += count;
              }
            }
          }
        }
      }
    }
  }
}
