#pragma once

#include "DocRenderer.hpp"

#include <StyleOption.hpp>

#include <memory>

std::unique_ptr<DocRendererFactory> make_doc_renderer(StyleOption style);
