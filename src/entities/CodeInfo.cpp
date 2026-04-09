#include "CodeInfo.hpp"

CodeInfo::CodeInfo(const std::string& name, const std::string& decl, const std::filesystem::path& path, uint32_t start_line, uint32_t end_line, std::string_view docs, bool visible)
    : name(name), decl(decl), path(path), start_line(start_line), end_line(end_line), docs(docs), visible(visible) {}
