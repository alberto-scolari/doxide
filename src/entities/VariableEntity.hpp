#pragma once

#include "TemplateCodeEntity.hpp"

class VariableEntity : public TemplateCodeEntity<false> {
public:
    using TemplateCodeEntity<false>::TemplateCodeEntity;
};
