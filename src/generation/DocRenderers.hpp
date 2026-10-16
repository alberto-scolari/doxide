#pragma once

#include "DocRenderer.hpp"

#include <string_view>
#include <memory>

std::unique_ptr<DocRendererFactory> make_doc_renderer(std::string_view style);
