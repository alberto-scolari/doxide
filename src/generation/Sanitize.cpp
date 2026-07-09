#include "Sanitize.hpp"

#include "Regex.hpp"

#include <iomanip>
#include <iterator>
#include <regex>
#include <sstream>

// TODO: optimize the following sanitization functions, e.g.,
// by wrapping the input in a custom type and adding custom formatters, like
//
// // 1. Define the wrapper object
// struct RegexReplacer {
//     std::string_view text;
//     std::regex pattern;
//     std::string_view replacement;
// };

// // 2. Specialize std::formatter for the wrapper object
// template <>
// struct std::formatter<RegexReplacer> : std::formatter<std::string_view> {
//     // Inherit parse() from string_view formatter, or define your own if you want custom flags

//     auto format(const RegexReplacer& obj, std::format_context& ctx) const {
//         // ctx.out() provides an output iterator for the formatting target (e.g., stdout, file, or string buffer)
//         // std::regex_replace accepts this iterator directly.
//         return std::regex_replace(
//             ctx.out(),
//             obj.text.begin(),
//             obj.text.end(),
//             obj.pattern,
//             std::string(obj.replacement) // std::regex_replace requires a std::string or const char* for formatting
//         );
//     }
// };

std::string sanitize(const std::string_view str) {
  static const std::regex word("\\w|[./\\\\]", REGEX_FLAGS);
  static const std::regex space("\\s", REGEX_FLAGS);

  std::stringstream buf;
  for (auto iter = str.begin(); iter != str.end(); ++iter) {
    if (std::regex_match(iter, iter + 1, word)) {
      buf << *iter;
    } else if (std::regex_match(iter, iter + 1, space)) {
      // skip whitespace
    } else {
      /* encode non-word and non-space characters */
      buf << "_u" << std::setfill('0') << std::setw(4) << std::hex << int(*iter);
    }
  }

  /* on Linux and Mac, the maximum file name length is 255 bytes, plus leave
   * room for a four-character file extension (e.g. .html); on Windows it is
   * 260 bytes, so use the minimum */
  return buf.str().substr(0, 255 - 5);
}

// replace every newline (possibly with spaces around) with a single space
std::string line(std::string_view str) {
  std::string result;
  static const std::regex newline("\\s*\\n\\s*", REGEX_FLAGS);
  std::regex_replace(std::back_inserter(result), str.cbegin(), str.cend(), newline, " ");
  return result;
}

std::string htmlize(const std::string& str) {
  /* basic replacements */
  static const std::regex amp("&", REGEX_FLAGS);
  static const std::regex lt("<", REGEX_FLAGS);
  static const std::regex gt(">", REGEX_FLAGS);
  static const std::regex quot("\"", REGEX_FLAGS);
  static const std::regex apos("'", REGEX_FLAGS);
  static const std::regex ptr("\\*", REGEX_FLAGS);

  /* the sequence operator[](...) looks like a link in Markdown */
  static const std::regex operator_brackets("operator\\[\\]", REGEX_FLAGS);

  std::string r = str;
  r = std::regex_replace(r, amp, "&amp;");  // must go first or new & replaced
  r = std::regex_replace(r, lt, "&lt;");
  r = std::regex_replace(r, gt, "&gt;");
  r = std::regex_replace(r, quot, "&quot;");
  r = std::regex_replace(r, apos, "&apos;");
  r = std::regex_replace(r, ptr, "&#42;");
  r = std::regex_replace(r, operator_brackets, "operator&#91;&#93;");
  return r;
}

std::string indent(const std::string& str) {
  static const std::regex start("\\n", REGEX_FLAGS);
  return "    " + std::regex_replace(str, start, "\n    ");
}
