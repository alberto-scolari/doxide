#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "TypeEntity.hpp"
#include "VariableEntity.hpp"
#include "FunctionEntity.hpp"
#include "EnumEntity.hpp"
#include "NamespaceEntity.hpp"

class RootEntity: public TypeContainer, public VariableContainer, public FunctionContainer, public EnumContainer, public NamespaceContainer {

  public:
  RootEntity(const std::string& title, std::string_view docs, const std::vector<std::string>& groups);

private:
  std::string title;
  std::string_view docs;
  std::vector<std::string> groups;
  bool visible;

};
