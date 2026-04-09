#pragma once

#include <vector>

#include "CodeInfo.hpp"

class VariableEntity : public CodeInfo {
public:
    VariableEntity(const std::string& name, const std::string& decl, const std::filesystem::path& path, uint32_t start_line, uint32_t end_line, std::string_view docs, bool visible);
};

class VariableContainer {
protected:
  std::vector<VariableEntity> variables;

  VariableContainer() = default;

public:
  inline const std::vector<VariableEntity>& get_variables() const noexcept {
    return variables;
  }

  inline void add_variable(VariableEntity&& variable) {
    variables.push_back(std::move(variable));
  }
};
