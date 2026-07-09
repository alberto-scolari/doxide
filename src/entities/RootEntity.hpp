#pragma once

#include "Documentable.hpp"
#include "Titled.hpp"
#include "ContainerBases.hpp"

class RootEntity: public Titled, public Documentable, public GroupContainer, public TypeContainer, public VariableContainer, public FunctionContainer, public EnumContainer, public NamespaceContainer, public ConceptContainer, public OperatorContainer, public TypedefContainer, public MacroContainer {
public:
  RootEntity(std::string_view title, std::string_view docs): Titled("", title), Documentable(this->stored_name, docs, true) {}
};
