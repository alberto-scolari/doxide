#pragma once

#include "ContainerBases.hpp"
#include "TemplateCodeEntity.hpp"

class FunctionEntity : public TemplateCodeEntity<false>, public TypeContainer, public FunctionContainer, public VariableContainer, public EnumContainer, public TypedefContainer, public OperatorContainer {
public:
  using TemplateCodeEntity<false>::TemplateCodeEntity;

};
