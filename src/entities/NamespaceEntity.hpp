#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

#include "TypeEntity.hpp"
#include "VariableEntity.hpp"
#include "FunctionEntity.hpp"
#include "EnumEntity.hpp"

template<typename T> class NamespaceContainerBase {
protected:
  std::vector<T> namespaces;

  NamespaceContainerBase() = default;

public:
  inline const std::vector<T>& get_namespaces() const noexcept {
    return namespaces;
  }

  inline void add_namespace(T&& namespace_entity) {
    namespaces.push_back(std::move(namespace_entity));
  }
};

class NamespaceEntity: public TypeContainer, public VariableContainer, public FunctionContainer, public EnumContainer, public NamespaceContainerBase<NamespaceEntity> {
public:
  NamespaceEntity(const std::string& name, uint32_t start_line, uint32_t end_line, std::string_view docs);

private:
  std::string name;
  std::string decl;
  uint32_t start_line;
  uint32_t end_line;
  std::string_view docs;
  bool visible;
};

using NamespaceContainer = NamespaceContainerBase<NamespaceEntity>;
