#pragma once

#include "Documentable.hpp"
#include "Titled.hpp"
#include "ContainerBases.hpp"

#include <Doc.hpp>

class GroupEntity: public Titled, public Documentable, public TypeContainer, public VariableContainer, public FunctionContainer, public EnumContainer, public NamespaceContainer, public ConceptContainer, public OperatorContainer, public TypedefContainer, public MacroContainer  {
public:
  GroupEntity(std::string_view name, std::string_view title, std::string_view description):
    Titled(name, title), Documentable(this->stored_name, Doc(description), true) {}
};
