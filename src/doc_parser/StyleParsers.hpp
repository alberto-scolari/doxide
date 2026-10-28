#pragma once

#include "DocParser.hpp"

#include <StyleOption.hpp>

#include <memory>

std::unique_ptr<DocParser> make_doc_parser(StyleOption style);
