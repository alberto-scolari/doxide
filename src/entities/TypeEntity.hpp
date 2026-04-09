#pragma once

#include <vector>

#include "CodeInfo.hpp"
#include "FunctionEntity.hpp"
#include "VariableEntity.hpp"
#include "EnumEntity.hpp"

template<typename T> class TypeContainerBase {
protected:
  std::vector<T> types;

  TypeContainerBase() = default;

public:
  inline const std::vector<T>& get_types() const noexcept {
    return types;
  }

  inline void add_type(T&& type) {
    types.push_back(std::move(type));
  }
};

class TypeEntity : public CodeInfo, public TypeContainerBase<TypeEntity>, public FunctionContainer, public VariableContainer, public EnumContainer {
public:
  // we can do it because CodeInfo is the only base class with a non-default constructor
  using CodeInfo::CodeInfo;
};

using TypeContainer = TypeContainerBase<TypeEntity>;
