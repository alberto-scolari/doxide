#pragma once

#include <Sink.hpp>
#include <entities/FwdRef.hpp>
#include <entities/Entities.hpp>

#include <string_view>
#include <vector>

class DocRenderer {
public:

  virtual void render_groups(const std::vector<FwdRef<GroupEntity>>& groups) = 0;

  virtual void render_ns(const std::vector<FwdRef<NamespaceEntity>>& ns) = 0;

  virtual void print_coverage(std::string_view dir) = 0;

  virtual void render_enums(const std::vector<FwdRef<EnumEntity>>& ns) = 0;

  virtual void render_details(const std::vector<FwdRef<TypedefEntity>>& td) = 0;

  virtual void render_details(const std::vector<FwdRef<ConceptEntity>>& td) = 0;

  virtual void render_details(const std::vector<FwdRef<MacroEntity>>& td) = 0;

  virtual void render_details(const std::vector<FwdRef<VariableEntity>>& td) = 0;

  virtual void render_details(const std::vector<FwdRef<OperatorEntity>>& td) = 0;

  virtual void render_details(const std::vector<FwdRef<FunctionEntity>>& td) = 0;

  virtual ~DocRenderer() = default;
};

class DocRendererFactory {
public:

  virtual DocRenderer& get_renderer(sink_t& sink) = 0;

  virtual ~DocRendererFactory() = default;
};
