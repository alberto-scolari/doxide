#include "NamespaceEntity.hpp"

NamespaceEntity::NamespaceEntity(const std::string& name, uint32_t start_line, uint32_t end_line, std::string_view docs) :
    name(name),
    start_line(start_line),
    end_line(end_line),
    docs(docs),
    visible(true) {}
