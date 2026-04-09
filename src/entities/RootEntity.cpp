#include "RootEntity.hpp"

RootEntity::RootEntity(const std::string& title, std::string_view docs, const std::vector<std::string>& groups) :
    title(title), docs(docs), groups(groups), visible(true) {
}
