#pragma once

#include "TextLineCursor.hpp"

#include <string>
#include <string_view>
#include <utility>

/**
 * Documentation of an entity.
 *
 * @ingroup developer
 */
struct Doc {
  /**
   * Constructor.
   *
   * @param comment Comment from which to populate documentation.
   * @param init_indent Initial indent level.
   */

  explicit constexpr Doc(const std::string_view _docs): docs(_docs), hide(false) {}

  constexpr Doc(const Doc&) = default;

  constexpr Doc(Doc&&) = default;

  constexpr Doc(): hide(false) {}

  constexpr Doc& operator=(const Doc&) = delete;

  constexpr Doc& operator=(Doc&&) = default;

  constexpr void drop_group() { ingroup = TextLineCursor(); }

  /**
   * Content of the documentation.
   */
  std::string docs;

  /**
   * Group to which the entity belongs, obtained from @ingroup in the
   * documentation comment.
   */
  TextLineCursor ingroup;

  /**
   * Hide the associated entity?
   */
  bool hide;

  static Doc from_comment(const TextLineCursor &comment);

private:
  constexpr Doc(std::string&& _docs, TextLineCursor&& _ingroup, bool _hide):
    docs(std::move(_docs)), ingroup(std::move(_ingroup)), hide(_hide) {}
};
