#pragma once

#include "TemplateCodeEntity.hpp"
#include "ContainerBases.hpp"

class TypeEntity : public TemplateCodeEntity<false>, public TypeContainer, public FunctionContainer, public VariableContainer, public EnumContainer, public TypedefContainer, public OperatorContainer {
public:
  using TemplateCodeEntity<false>::TemplateCodeEntity;
};
