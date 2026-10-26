#pragma once

#include "DocParser.hpp"

#include <memory>
#include <string_view>

std::unique_ptr<DocParser> make_doc_parser(std::string_view style);
