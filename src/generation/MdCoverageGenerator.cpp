#include "MdCoverageGenerator.hpp"

#include <FileSystemDeleter.hpp>
#include <FileSystemStatsInfo.hpp>
#include <Sink.hpp>
#include <Utils.hpp>
#include "entities/FileEntity.hpp"
#include <entities/FwdRef.hpp>
#include "entities/LineStats.hpp"
#include <entities/RefVariant.hpp>

#include "Dfs_traversal.hpp"
#include "MdCommon.hpp"
#include "Sanitize.hpp"

#include <cstddef>
#include <fstream>
#include <stack>
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <utility>

#include <iostream>

static constexpr std::string_view SUNBURST_PRELUDE = R""""(
<div id="coverage-sunburst" style="aspect-ratio:1/1;width:100%;max-width:600px;margin:auto;"></div>
<script type="text/javascript">
var data = [)"""";

static constexpr std::string_view SUNBURST_POSTLUDE = R""""(]

var coverage_root = "";  // current root of coverage report

function update_coverage_table(params) {
  if (typeof params.data.name === 'undefined') {
    // occurs when the central circle is selected to go up one level
    var path = coverage_root.substring(0, coverage_root.lastIndexOf('/'));
    var is_dir = true;
  } else {
    var path = params.data.path;
    var is_dir = params.data.type === 'dir';
  }
  let rows = document.querySelectorAll('[data-parent]');
  if (is_dir) {
    for (let row of rows) {
      if (row.dataset.parent === path) {
        row.style.display = '';
      } else {
        row.style.display = 'none';
      }
    }
  } else {
    for (let row of rows) {
      if (row.id === path) {
        row.style.display = '';
      } else {
        row.style.display = 'none';
      }
    }
  }
  coverage_root = path;
}

window.addEventListener("load", () => {
  var coverage_sunburst = echarts.init(document.getElementById('coverage-sunburst'));
  var option = {
    series: {
      type: 'sunburst',
      data: data,
      sort: null,
      radius: ['5%', '95%'],
      startAngle: 0,
      clockwise: false,
      itemStyle: {
        borderWidth: 1
      },
      label: {
        color: 'white',
        fontSize: 10,
        textBorderWidth: 1,
        align: 'center',
        rotate: 'radial',
        width: 80,
        minAngle: 4,
        overflow: 'truncate',
        formatter: function (params) {
          if (params.data.icon) {
            return params.name + '\n' + params.data.icon;
          } else {
            return params.name;
          }
        }
      },
      labelLayout: {
        hideOverlap: true
      },
      levels: [
        {
          itemStyle: {
            opacity: 0.2
          }
        }
      ]
    }
  };
  coverage_sunburst.setOption(option);
  coverage_sunburst.on('click', update_coverage_table);
  window.addEventListener("resize", () => {
    coverage_sunburst.resize();
  });
});

window.addEventListener("load", () => {
  var tables = document.querySelectorAll("article table:not([class])");
  tables.forEach(function(table) {
    new Tablesort(table);
  });
});

</script>
)"""";


static constexpr std::string_view TABLE_COVERAGE_0 = R""""(
<table>
<thead>
<tr>
<th style="text-align:left;" data-sort-method="dotsep">Name</th>
<th style="text-align:right;" data-sort-method="number">Lines</th>
<th style="text-align:right;" data-sort-method="number">Covered</th>
<th style="text-align:right;" data-sort-method="number">Uncovered</th>
<th style="text-align:right;" data-sort-method="number">Coverage</th>
</tr>
</thead>
<tbody>
)"""";

static constexpr std::string_view TABLE_COVERAGE_1 = R""""(</tbody>
<tfoot>
)"""";

static constexpr std::string_view TABLE_COVERAGE_2 = R""""(</tfoot>
</table>
)"""";

static constexpr std::string_view material_file_outline = "<span class=\"twemoji\"><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 24 24\"><path d=\"M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8zm4 18H6V4h7v5h5z\"/></svg></span>";

static constexpr std::string_view material_folder = "<span class=\"twemoji\"><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 24 24\"><path d=\"M10 4H4c-1.11 0-2 .89-2 2v12a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-8z\"/></svg></span>";

struct DisplayStats {
  double percent;
  std::string_view color;
  std::string_view icon;

  DisplayStats(const LineStats& s): percent(get_percent(s)),
  color(get_color(percent)), icon(get_icon(percent))
  {}

  static constexpr double get_percent(const LineStats& s) {
    return s.included_lines > 0 ? 100.0 * s.covered_lines / s.included_lines : 100.0;
  }

  static constexpr std::string_view get_color(double percent) {
    constexpr std::string_view red = "ef5552";
    constexpr std::string_view orange = "f78b2b";
    constexpr std::string_view amber = "ffc105";
    constexpr std::string_view olive = "a5b72a";
    constexpr std::string_view green = "4cae4f";
    if (percent < 60.0) {
      return red;
    } else if (percent < 70.0) {
      return orange;
    } else if (percent < 80.0) {
      return amber;
    } else if (percent < 90.0) {
      return olive;
    } else {
      return green;
    }
  }

  static constexpr std::string_view get_icon(double percent) {
    // assert(0.0 <= percent && percent <= 100.0);
    constexpr std::string_view icon0 = "○○○○";
    constexpr std::string_view icon1 = "●○○○";
    constexpr std::string_view icon2 = "●●○○";
    constexpr std::string_view icon3 = "●●●○";
    constexpr std::string_view icon4 = "●●●●";
    if (percent < 60.0) {
      return icon0;
    } else if (percent < 70.0) {
      return icon1;
    } else if (percent < 80.0) {
      return icon2;
    } else if (percent < 90.0) {
      return icon3;
    } else {
      return icon4;
    }
  }
};

std::string_view relative_path(std::string_view path, std::string_view base) {
  if (base.empty() or base.empty()) {
    return path;
  }
  if (base.size() >= path.size()) {
    return "";
  }
  return path.substr(base.size() + 1);
}

std::string_view parent_path(std::string_view path) {
  size_t last_slash = path.find_last_of("/\\");
  if (last_slash == std::string_view::npos) {
      return ""; // No slash found, the entire path is the filename
  }
  return path.substr(0, last_slash);
}

static void render_entry(sink_t& sink, const LineStats& stats, char letter, std::string_view root,
  std::string_view dirpath, std::string_view name, std::string_view material) {
  std::string parent = sanitize(relative_path(parent_path(dirpath), root));
  std::string sname = sanitize(name);
  DisplayStats ds(stats);
  std::string_view style = parent.empty() ? "" : " style=\"display:none;\"";
  std::string path = parent.empty() ? sname : parent + '/' + sname;
  std::size_t lines_uncovered = stats.included_lines - stats.covered_lines;

  sink.print(R""""(<tr id="{0}" data-parent="{1}"{2}>
<td style="text-align:left;" data-sort="{3}.{4}">{5} <a href="{0}/">{6}</a></td>
<td style="text-align:right;">{7}</td>
<td style="text-align:right;">{8}</td>
<td style="text-align:right;">{9}</td>
<td style="text-align:right;box-shadow: -8px 0 0 0 #{10}dd inset;">{11:.1f}%</td>
</tr>
)"""", path, parent, style, letter, sname, material, htmlize(sname),
    stats.included_lines, stats.covered_lines, lines_uncovered, ds.color, ds.percent
  );
}

struct CoverageDfs: public ConstDirStackController {
  using stack_value = ConstDirStackController::stack_value;
  using yield_value = ConstDirStackController::yield_value;

  sink_t& sink;
  std::string_view root;
  std::stack<std::pair<double, bool>> percent_stack;

  void before_pop(const stack_value& top) {
    percent_stack.pop();
    const auto& dirinfo = std::get<1>(top);
    for(const auto& fe: dirinfo.files) {
      render_entry(sink, *fe, 'b', root, fe->get_path(),
        fe->get_filename(), material_file_outline);
    }
  }

  stack_value make_next(stack_value& top) {
    double perc = DisplayStats::get_percent(std::get<1>(top).stats);
    bool is_first = std::get<2>(top) == std::get<1>(top).subdirs.cbegin();
    percent_stack.emplace(perc, is_first);
    return ConstDirStackController::make_next(top);
  }
};

void MdCoverageGenerator::coverage_data(GenInfo& info) {
  std::string_view root = info.is_not_root ? info.dirpath : "";
  CoverageDfs controller{{info.fsinfo, info.dirpath}, info.sink, root, {}};
  controller.percent_stack.emplace(-1.0, false);
  for (const auto& [dirpath, dirinfo]: traverse_dfs_preorder(controller)) {
    if (dirpath == info.dirpath) {
      continue;
    }
    render_entry(info.sink, dirinfo.stats, 'a', root, dirpath,
      FileStorage::get_filename(dirpath), material_folder);
  }
}

void MdCoverageGenerator::coverage_foot(GenInfo& info) {
  std::string_view root = info.is_not_root ? info.dirpath : "";
  ConstDirStackController controller{info.fsinfo, info.dirpath};
  for (const auto& [dirpath, dirinfo]: traverse_dfs_preorder<ConstDirStackController, false>(controller)) {
    std::string_view path = dirpath != FileSystemStatsInfo::DEF_ROOT_PATH ? dirpath : "";
    std::string name = sanitize(path);
    std::string_view style = (name == root) ? "" : ", style=\"display:none;\"";
    const auto& stats = dirinfo.stats;
    DisplayStats ds(stats);
    std::size_t lines_uncovered = stats.included_lines - stats.covered_lines;

    info.sink.print(R""""(<tr id="summary.{0}" data-parent="{0}"{1}>
<td style="text-align:left;font-weight:bold;">Summary</td>
<td style="text-align:right;font-weight:bold;">{2}</td>
<td style="text-align:right;font-weight:bold;">{3}</td>
<td style="text-align:right;font-weight:bold;">{4}</td>
<td style="text-align:right;font-weight:bold;box-shadow: -8px 0 0 0 #{5}dd inset;">{6:.1f}%</td>
</tr>
)"""",
      name, style, stats.included_lines, stats.covered_lines,
      lines_uncovered, ds.color, ds.percent);
  }
}

struct SunburstDfs: public ConstDirStackController {
  using stack_value = ConstDirStackController::stack_value;
  using yield_value = ConstDirStackController::yield_value;

  sink_t& sink;
  std::string_view root;
  std::stack<std::pair<double, bool>> percent_stack;

  void before_pop(const stack_value& top) {
    std::string_view color = DisplayStats::get_color(percent_stack.top().first);
    percent_stack.pop();
    const auto& dirinfo = std::get<1>(top);
    bool first = dirinfo.subdirs.empty();
    if (not first) {
      sink.print(
        "], itemStyle: {{ color: \"#{0}dd\", borderColor: \"#{0}\"}}, "
        "label: {{ textBorderColor: \"#{0}\"}}"
        "}}", color);
    }
    std::size_t rel_beginning = not root.empty() ? root.size() + 1 : 0;
    for(const auto& fe: dirinfo.files) {
      DisplayStats ds(*fe);
      std::string_view relative_path = fe->get_path().substr(rel_beginning);
      if (not first) {
        sink.print(", ");
      }
      first = false;
      sink.print(
        "{{"
        "name: \"{0}\", path: \"{1}\", value: {2}, type: \"file\", icon: \"{3}\", "
        "itemStyle: {{ color: \"#{4}dd\", borderColor: \"#{4}\"}}, "
        "label: {{ textBorderColor: \"#{4}\"}}"
        "}}", fe->get_filename(), relative_path, fe->get_included_lines(), ds.icon, ds.color);
    }
  }

  stack_value make_next(stack_value& top) {
    double perc = DisplayStats::get_percent(std::get<1>(top).stats);
    bool is_first = std::get<2>(top) == std::get<1>(top).subdirs.cbegin();
    percent_stack.emplace(perc, is_first);
    return ConstDirStackController::make_next(top);
  }
};

void MdCoverageGenerator::sunburst(GenInfo& info) {
    info.sink.print("{}", SUNBURST_PRELUDE);
    std::string_view root = info.is_not_root ? info.dirpath : "";
    SunburstDfs controller{{info.fsinfo, info.dirpath}, info.sink, root, {}};
    controller.percent_stack.emplace(-1.0, false);
    for (const auto& [dirpath, dirinfo]: traverse_dfs_preorder(controller)) {
      if (dirpath == info.dirpath) {
        continue;
      }
      if (not controller.percent_stack.top().second) {
        info.sink.print(", ");
      }
      std::string_view relative_path = info.is_not_root ? dirpath.substr(info.dirpath.size() + 1) : dirpath;
      std::string_view dirname = FileStorage::get_filename(relative_path);
      std::string_view icon = DisplayStats::get_icon(controller.percent_stack.top().first);
      info.sink.print("{{name: \"{}\", path: \"{}\", value: {},"
        " type: \"dir\", icon: \"{}\", children: [",
        dirname, relative_path, dirinfo.stats.included_lines, icon
      );
    }
    info.sink.print("{}", SUNBURST_POSTLUDE);
}

void MdCoverageGenerator::render_dir_coverage(GenInfo& info) {
  info.sink.println(FRONTMATTER);
  info.sink.println("# {}\n\n", info.title);

  sunburst(info);
  info.sink.print(TABLE_COVERAGE_0);
  coverage_data(info);
  info.sink.print(TABLE_COVERAGE_1);
  coverage_foot(info);
  info.sink.print(TABLE_COVERAGE_2);
  info.sink.println();
}

struct CoverageFileDfs: public ConstDirStackController {
  using stack_value = ConstDirStackController::stack_value;
  using yield_value = ConstDirStackController::yield_value;

  const std::filesystem::path& output;
  FileSystemDeleter& fscleaner;

  void before_pop(const stack_value& top) {
    static constexpr std::string_view covered_color = DisplayStats::get_color(100.0);
    static constexpr std::string_view uncovered_color = DisplayStats::get_color(0.0);

    const auto& dirinfo = std::get<1>(top);
    for(const auto& fe: dirinfo.files) {
      std::cout << "-- file: " << fe->get_path() << std::endl;
      std::filesystem::path outpath = output / (std::string(fe->get_path()) + ".md");
      std::filesystem::create_directories(outpath.parent_path());
      if (not can_write(outpath)) {
        continue;
      }
      fscleaner.add_file(outpath);
      std::ofstream out(outpath);
      sink_t sink(out);

      /* for a file, output the whole contents; lines are highlighted with
       * CSS; two styles are given, one that works with MkDocs output, one
       * that works with pandoc output */

      sink.println(FRONTMATTER);
      sink.println("# {}\n", fe->get_name());
      sink.println("{}\n", fe->get_docs());
      /* for MkDocs, line numbers are added with the Markdown notation
        * `{linenums="1"}` below, which ultimately creates a HTML <table> of
        * one row <tr>, with two cells <td>, the first holding the line
        * numbers and the second the code; the line number cell contains a
        * further <pre> with one empty <span></span>, then each line number
        * in a separate <span> within it; to indicate code coverage we apply
        * styles to those <span>, which are readily selected by number using
        * the CSS nth-child() pseudo-class */
      sink.println(R""""(<style>
.linenodiv pre span {{padding-left:4px;padding-right:4px}})"""");

      const auto& line_counts = fe->get_line_counts();
      for (std::size_t line = 0; line < line_counts.size(); ++line) {
        if (line_counts[line] < 0) {
          continue;
        }

        bool pos = line_counts[line] > 0;
        std::string_view color = pos ? covered_color : uncovered_color;
        std::string_view dot = pos ? "●" : "○";
        sink.println(
          R""""(.linenodiv pre span:nth-child({0}) {{background-color: #{1}dd;color: white;}}
  .linenodiv pre span:nth-child({0})::before {{content: "{2} ";}})"""",
          line + 2, color, dot);
      }

      /* for pandoc, line numbers are added through the CSS; there is a <span>
       * per line that is readily selectable with nth-child(), and it contains
       * an <a> at the start that is expanded to add the line number and
       * colour it */
      sink.println(R""""(.sourceCode pre span a {{padding-left:8px;padding-right:8px;text-decoration:none;}})"""");
      for (std::size_t line = 0; line < line_counts.size(); ++line) {
        const bool bigger = line_counts[line] > 0;
        const bool zero = line_counts[line] == 0;
        std::string_view dot = bigger ? "●" : (zero ? "○" : " ");
        sink.print(".sourceCode pre span:nth-child({}) a {{", line + 1);
        if (bigger or zero) {
          std::string_view color = bigger ? covered_color : uncovered_color;
          sink.print("background-color: #{}dd;", color);
        }
        const std::string_view color = (bigger or zero) ? "white" : "currentColor";
        sink.println(R""""(color: {};}}
.sourceCode pre span:nth-child({}) a::before {{content: "{} {:4}";}})"""",
          color, line + 1, dot, line);
      }
      sink.println(
R""""(</style>

```cpp {{linenums="1"}}
{}
```
)"""", fe->get_content());
    }
  }
};

void MdCoverageGenerator::generate(const FileSystemStatsInfo& fsinfo) {
  CoverageFileDfs controller{{fsinfo}, output, fscleaner};
  for (const auto& [dirpath, dirinfo]: traverse_dfs_preorder(controller)) {
    std::cout << "-- dir: " << dirpath << std::endl;
    std::filesystem::path outpath = output;
    const bool is_not_root = dirpath != FileSystemStatsInfo::DEF_ROOT_PATH;
    if (is_not_root) {
      outpath /= dirpath;
    }

    std::filesystem::create_directories(outpath);
    std::filesystem::path file = outpath / "index.md";
    if (not can_write(file)) {
      continue;
    }
    fscleaner.add_file(file);
    std::ofstream out(file);
    sink_t sink(out);
    std::string_view title = is_not_root?  FileStorage::get_filename(dirpath) : "Code Coverage";
    GenInfo info{fsinfo, sink, dirpath, dirinfo, title, is_not_root};
    render_dir_coverage(info);
  }
}
