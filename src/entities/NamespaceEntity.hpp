#pragma once

#include "CodeEntity.hpp"
#include "ContainerBases.hpp"

class NamespaceEntity: public CodeEntity, public TypeContainer, public VariableContainer, public FunctionContainer, public EnumContainer, public NamespaceContainer, public ConceptContainer, public OperatorContainer, public TypedefContainer, public MacroContainer {
public:
  using CodeEntity::CodeEntity;
};
