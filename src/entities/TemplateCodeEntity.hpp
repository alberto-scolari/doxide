#pragma once

#include "CodeEntity.hpp"
#include "FileEntity.hpp"
#include "FwdRef.hpp"

#include <format>
#include <stdexcept>
#include <utility>

template<bool _is_template> class TemplateCodeEntity: public CodeEntity {
public:
  constexpr TemplateCodeEntity(std::string_view template_decl, std::string_view entity_decl, std::string_view name, const FwdRef<FileEntity>& file, uint32_t start_line, uint32_t end_line, const Doc& docs, bool visible):
    CodeEntity(not template_decl.empty() ? std::format("{} {}", template_decl, entity_decl) : std::string(entity_decl),
      name, file, start_line, end_line, docs, visible), _template_decl(this->get_decl().data(), template_decl.size()) {
        if (_is_template and template_decl.empty()) {
          throw std::runtime_error(std::format("Entity named \"{}\" should be templated, but it is not", name));
        }
      }

  constexpr TemplateCodeEntity(std::string_view template_decl, std::string_view entity_decl, std::string_view name, const FwdRef<FileEntity>& file, uint32_t start_line, uint32_t end_line, Doc&& docs, bool visible):
    CodeEntity(not template_decl.empty() ? std::format("{} {}", template_decl, entity_decl) : std::string(entity_decl),
      name, file, start_line, end_line, std::move(docs), visible), _template_decl(this->get_decl().data(), template_decl.size()) {
        if (_is_template and template_decl.empty()) {
          throw std::runtime_error(std::format("Entity named \"{}\" should be templated, but it is not", name));
        }
      }

  constexpr bool is_template() const noexcept {
    return not get_template_decl().empty();
  }

  constexpr std::string_view get_template_decl() const noexcept {
    return _template_decl;
  }

protected:
  std::string_view _template_decl;
};
