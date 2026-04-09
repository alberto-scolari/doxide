#pragma once

#include <vector>

#include "CodeInfo.hpp"

class FunctionEntity : public CodeInfo {
public:
   using CodeInfo::CodeInfo;
};

class FunctionContainer {
protected:
  std::vector<FunctionEntity> functions;

  FunctionContainer() = default;

public:
  inline const std::vector<FunctionEntity>& get_functions() const noexcept {
    return functions;
  }

  inline void add_function(FunctionEntity&& function) {
    functions.push_back(std::move(function));
  }
};
