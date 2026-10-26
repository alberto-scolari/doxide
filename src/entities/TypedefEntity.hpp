#pragma once

#include "TemplateCodeEntity.hpp"

class TypedefEntity : public TemplateCodeEntity<false> {
public:
  using TemplateCodeEntity<false>::TemplateCodeEntity;
};
