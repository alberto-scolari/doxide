#include "YAMLNode.hpp"

#include <utility>

bool YAMLNode::has(const key_view_type& key) const {
  return isMapping() && mapping().contains(key);
}

bool YAMLNode::isValue() const {
  return std::holds_alternative<value_type>(contents);
}

bool YAMLNode::isSequence() const {
  return std::holds_alternative<sequence_type>(contents);
}

bool YAMLNode::isMapping() const {
  return std::holds_alternative<mapping_type>(contents);
}

const YAMLNode::value_type& YAMLNode::value() const {
  return std::get<value_type>(contents);
}

YAMLNode::sequence_type& YAMLNode::sequence() {
  return std::get<sequence_type>(contents);
}

const YAMLNode::sequence_type& YAMLNode::sequence() const {
  return std::get<sequence_type>(contents);
}

YAMLNode::mapping_type& YAMLNode::mapping() {
  return std::get<mapping_type>(contents);
}

const YAMLNode::mapping_type& YAMLNode::mapping() const {
  return std::get<mapping_type>(contents);
}

void YAMLNode::set(variant_type&& contents) {
  this->contents = std::move(contents);
}

void YAMLNode::setSequence() {
  contents = sequence_type{};
}

void YAMLNode::setMapping() {
  contents = mapping_type{};
}

YAMLNode& YAMLNode::push() {
  return *sequence().emplace_back(std::make_unique<YAMLNode>());
}

YAMLNode& YAMLNode::insert(key_type&& key) {
  return *mapping().emplace(std::move(key),
      std::make_unique<YAMLNode>()).first->second;
}
