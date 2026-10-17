#pragma once

#include "Doc.hpp"
#include "TextLineCursor.hpp"

class DocParser {
public:
  virtual Doc parse(const TextLineCursor &comment) = 0;

  virtual ~DocParser() = default;
};
