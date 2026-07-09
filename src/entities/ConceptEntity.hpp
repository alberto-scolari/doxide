#pragma once

#include "TemplateCodeEntity.hpp"

class ConceptEntity : public TemplateCodeEntity<true> {
public:
  using TemplateCodeEntity<true>::TemplateCodeEntity;
};
