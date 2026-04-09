#pragma once

#include <vector>

#include "CodeInfo.hpp"

class EnumEntity : public CodeInfo {
public:
  using CodeInfo::CodeInfo;
};

class EnumContainer {
protected:
  std::vector<EnumEntity> enums;

  EnumContainer() = default;

public:
  inline const std::vector<EnumEntity>& get_enums() const noexcept {
    return enums;
  }

  inline void add_enum(EnumEntity&& enum_entity) {
    enums.push_back(std::move(enum_entity));
  }
};
