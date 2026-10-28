#include "StyleOption.hpp"

#include "Log.hpp"

StyleOption str_to_style_option(const std::string_view& style) {
  if (style == "mkdocs") {
    return StyleOption::MKDOCS;
  } else if (style == "pandoc") {
    return StyleOption::PANDOCS;
  } else if (style == "plain") {
    return StyleOption::PLAIN;
  }
  warn("unknown style '" << style << "', setting 'mkdocs'.");
  return StyleOption::MKDOCS;
}

std::string_view style_option_to_str(StyleOption style) {
  switch (style) {
    case StyleOption::MKDOCS: {
      return "mkdocs";
    }
    case StyleOption::PANDOCS: {
      return "pandoc";
    }
    case StyleOption::PLAIN: {
      return "plain";
    }
  }
}
