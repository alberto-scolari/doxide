#pragma once

#include "ConceptEntity.hpp"
#include "DirEntity.hpp"
#include "Entities.hpp"
#include "EnumEntity.hpp"
#include "FileEntity.hpp"
#include "FunctionEntity.hpp"
#include "FwdList.hpp"
#include "GroupEntity.hpp"
#include "MacroEntity.hpp"
#include "NamespaceEntity.hpp"
#include "OperatorEntity.hpp"
#include "RefVariant.hpp"
#include "RootEntity.hpp"
#include "TypeEntity.hpp"
#include "VariableEntity.hpp"
#include "TypedefEntity.hpp"

#include <filesystem>
#include <format>
#include <iterator>
#include <list>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <iostream>

class EntityRegistry {

  template<entity T> class entities_storage {
  public:
    using storage_t = std::list<std::vector<FwdRef<T>>>;
    using sequence_ref = storage_t::reference;
    using sequence_iter = storage_t::iterator;
    using const_view_t = std::remove_cvref_t<decltype(std::declval<const storage_t&>() | std::views::join)>;

    // initialize empty
    constexpr entities_storage() = default;

    sequence_iter back_iter() {
      if (data.empty()) {
        push_back_sequence();
      }
      return _back_iter();
    }

    sequence_iter back_new_iter() {
      if (data.empty() or not data.back().empty()) {
        push_back_sequence();
      }
      return _back_iter();
    }

    const_view_t get_view() const {
      return data | std::views::join;
    }

    static inline const_view_t get_empty_view() {
      return EMPTY | std::views::join;
    }

  private:
    inline static const storage_t EMPTY;

    void push_back_sequence() {
      data.push_back(std::vector<FwdRef<T>>());
    }

    sequence_iter _back_iter() {
      return std::prev(data.end());
    }

    storage_t data;
  };

public:

  template<entity T> using edges_map_t = std::unordered_map<FwdHashable, entities_storage<T>>;
  template<entity T> using children_view_t = entities_storage<T>::const_view_t;

private:
  template<entity ChildT> FwdList<ChildT>& _get_node_list() {
    if constexpr (std::same_as< ChildT, TypeEntity>) {
      return type_entities;
    } else if constexpr (std::same_as< ChildT, FunctionEntity>) {
      return function_entities;
    } else if constexpr (std::same_as< ChildT, VariableEntity>) {
      return variable_entities;
    } else if constexpr (std::same_as< ChildT, EnumEntity>) {
      return enum_entities;
    } else if constexpr (std::same_as< ChildT, NamespaceEntity>) {
      return namespace_entities;
    } else if constexpr (std::same_as< ChildT, TypedefEntity>) {
      return typedef_entities;
    } else if constexpr (std::same_as< ChildT, ConceptEntity>) {
      return concept_entities;
    } else if constexpr (std::same_as< ChildT, OperatorEntity>) {
      return operator_entities;
    } else /*if constexpr (std::same_as<ChildT, MacroEntity>)*/ {
      static_assert(std::same_as< ChildT, MacroEntity>);
      return macro_entities;
    }
  }

  template<entity ChildT> edges_map_t<ChildT>& _get_edge_list() {
    if constexpr (std::same_as<ChildT, TypeEntity>) {
      return type_edges;
    } else if constexpr (std::same_as<ChildT, FunctionEntity>) {
      return function_edges;
    } else if constexpr (std::same_as<ChildT, VariableEntity>) {
      return variable_edges;
    } else if constexpr (std::same_as<ChildT, EnumEntity>) {
      return enum_edges;
    } else if constexpr (std::same_as<ChildT, NamespaceEntity>) {
      return namespace_edges;
    } else if constexpr (std::same_as<ChildT, TypedefEntity>) {
      return typedef_edges;
    } else if constexpr (std::same_as<ChildT, ConceptEntity>) {
      return concept_edges;
    } else if constexpr (std::same_as<ChildT, OperatorEntity>) {
      return operator_edges;
    } else /*if constexpr (std::same_as<ChildT, MacroEntity>)*/ {
      static_assert(std::same_as<ChildT, MacroEntity>);
      return macro_edges;
    }
  }

  template<entity ChildT> const edges_map_t<ChildT>& _get_edge_list() const {
    if constexpr (std::same_as<ChildT, TypeEntity>) {
      return type_edges;
    } else if constexpr (std::same_as<ChildT, FunctionEntity>) {
      return function_edges;
    } else if constexpr (std::same_as<ChildT, VariableEntity>) {
      return variable_edges;
    } else if constexpr (std::same_as<ChildT, EnumEntity>) {
      return enum_edges;
    } else if constexpr (std::same_as<ChildT, NamespaceEntity>) {
      return namespace_edges;
    } else if constexpr (std::same_as<ChildT, TypedefEntity>) {
      return typedef_edges;
    } else if constexpr (std::same_as<ChildT, ConceptEntity>) {
      return concept_edges;
    } else if constexpr (std::same_as<ChildT, OperatorEntity>) {
      return operator_edges;
    } else /*if constexpr (std::same_as<ChildT, MacroEntity>)*/ {
      static_assert(std::same_as<ChildT, MacroEntity>);
      return macro_edges;
    }
  }

  template<entity ParentT, entity ChildT> requires container_of<ParentT, ChildT>
  children_view_t<ChildT> _get_children_view(const FwdRef<ParentT>& e) const {
    const edges_map_t<ChildT>& edges = _get_edge_list<ChildT>();
    auto it = edges.find(e);
    return it != edges.cend() ? it->second.get_view() : entities_storage<ChildT>::get_empty_view();
  }

  using subdir_map_t = std::unordered_map<std::string_view, FwdRef<DirEntity>>;

public:
  EntityRegistry() = delete;

  EntityRegistry(const std::string& title, const std::string& docs) {
    root.emplace_back(title, docs);
    group_edges.back_new_iter();
  }

  inline const FwdRef<RootEntity> get_root_ref() const noexcept {
    return static_cast<FwdRef<RootEntity>>(root.begin());
  }

  inline FwdRef<RootEntity> get_root_ref() noexcept {
    return static_cast<FwdRef<RootEntity>>(root.begin());
  }

  inline FwdRef<GroupEntity> add_group(auto&&... args) {
    FwdRef<GroupEntity> res = static_cast<FwdRef<GroupEntity>>(group_entities.emplace_back(std::forward<decltype(args)>(args)...));
    std::string_view name = res->get_name();
    auto [it, newly_inserted] = name_group_map.emplace(name, res);
    if (not newly_inserted) {
      group_entities.erase(res);
      throw std::runtime_error(std::format("Group named \"{}\" already exists.", name));
    }
    group_edges.back_new_iter()->push_back(res);
    return res;
  }

  class EntityFileInserter {
      template<entity ParentT, entity ChildT> requires container_of<ParentT, ChildT>
      FwdRef<ChildT> _add_child_to_parent(const FwdRef<ParentT>& parent, auto&&... args) {
        FwdRef<ChildT> result = static_cast<FwdRef<ChildT>>(
          registry._get_node_list<ChildT>().emplace_back(std::forward<decltype(args)>(args)...));
        _add_to_parent(parent, result);
        return result;
      }

      template<entity ParentT, entity ChildT> requires container_of<ParentT, ChildT>
      void _add_to_parent(const FwdRef<ParentT>& parent, FwdRef<ChildT>& child) {
        std::optional<typename entities_storage<ChildT>::sequence_iter> iter_ref;
        if constexpr (std::same_as<ParentT, RootEntity>) {
          auto& seq_opt = registry.name_root_map[file].get_entity_opt<ChildT>();
          if (not seq_opt) {
            // if it doesn't exist, generate one
            seq_opt = registry._get_edge_list<ChildT>()[parent].back_new_iter();
          }
          iter_ref = *seq_opt;
        } else if constexpr (std::same_as<ParentT, GroupEntity>) {
          auto& seq_opt = registry.filename_group_catalogs_map[file][parent->get_name()].template get_entity_opt<ChildT>();
          if (not seq_opt) {
            seq_opt = registry._get_edge_list<ChildT>()[parent].back_new_iter();
          }
          iter_ref = *seq_opt;
        } else {
          iter_ref = registry._get_edge_list<ChildT>()[parent].back_iter();
        }
        iter_ref.value()->push_back(child);
      }

      template<entity ChildT>
      FwdRef<ChildT> _add_child_to_parent_ref(const RefVariant& parent, auto&&... args) {
        FwdRef<ChildT> result = static_cast<FwdRef<ChildT>>(
          registry._get_node_list<ChildT>().emplace_back(std::forward<decltype(args)>(args)...));
        std::visit([this, &result]<entity ParentT>(const FwdRef<ParentT>& p) {
          if constexpr (not container_of<ParentT, ChildT>) {
            registry._get_node_list<ChildT>().erase(result);
            throw std::runtime_error(std::format("Cannot insert {} into {}", entity_name<ChildT>(), entity_name<ParentT>()));
          } else {
            this->_add_to_parent(p, result);
          }
        }, parent);
        return result;
      }

      public:
        EntityFileInserter(std::string_view f, EntityRegistry& r): file(f), registry(r) {}

        EntityFileInserter(EntityFileInserter&& other): file(other.file), registry(other.registry) {}

      template<entity ParentT> requires container_of<ParentT, TypeEntity>
      FwdRef<TypeEntity> add_type(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, TypeEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<TypeEntity> add_type(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<TypeEntity>(parent, std::forward<decltype(args)>(args)...);
      }


      template<entity ParentT> requires container_of<ParentT, FunctionEntity>
      FwdRef<FunctionEntity> add_function(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, FunctionEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<FunctionEntity> add_function(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<FunctionEntity>(parent, std::forward<decltype(args)>(args)...);
      }


      template<entity ParentT> requires container_of<ParentT, VariableEntity>
      FwdRef<VariableEntity> add_variable(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, VariableEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<VariableEntity> add_variable(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<VariableEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      template<entity ParentT> requires container_of<ParentT, EnumEntity>
      FwdRef<EnumEntity> add_enum(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, EnumEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<EnumEntity> add_enum(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<EnumEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      template<entity ParentT> requires container_of<ParentT, NamespaceEntity>
      FwdRef<NamespaceEntity> add_namespace(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, NamespaceEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<NamespaceEntity> add_namespace(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<NamespaceEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      template<entity ParentT> requires container_of<ParentT, TypedefEntity>
      FwdRef<TypedefEntity> add_typedef(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, TypedefEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<TypedefEntity> add_typedef(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<TypedefEntity>(parent, std::forward<decltype(args)>(args)...);
      }


      template<entity ParentT> requires container_of<ParentT, ConceptEntity>
      FwdRef<ConceptEntity> add_concept(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, ConceptEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<ConceptEntity> add_concept(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<ConceptEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      template<entity ParentT> requires container_of<ParentT, OperatorEntity>
      FwdRef<OperatorEntity> add_operator(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, OperatorEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<OperatorEntity> add_operator(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<OperatorEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      template<entity ParentT> requires container_of<ParentT, MacroEntity>
      FwdRef<MacroEntity> add_macro(const FwdRef<ParentT>& parent, auto&&... args) {
        return _add_child_to_parent<ParentT, MacroEntity>(parent, std::forward<decltype(args)>(args)...);
      }

      FwdRef<MacroEntity> add_macro(const RefVariant& parent, auto&&... args) {
        return _add_child_to_parent_ref<MacroEntity>(parent, std::forward<decltype(args)>(args)...);
      }

    private:
      std::string_view file;
      EntityRegistry& registry;
  };

  friend EntityFileInserter;

  EntityFileInserter get_inserter(const FwdRef<FileEntity>& file) {
    return EntityFileInserter(file->get_path(), *this);
  }

  FwdRef<FileEntity> add_file(const std::filesystem::path& path, auto&&... args) {
    if (path.empty() or not path.has_filename()) {
      throw std::runtime_error(std::format("empty file path or no filename"));
    }
    namespace fs = std::filesystem;
    auto dir_try_emplace = [&](subdir_map_t& map, std::string_view pathname) -> FwdRef<DirEntity> {
        auto it = map.find(pathname);
        if (it != map.end()) {
            return it->second;
        }
        FwdRef<DirEntity> r = *directories.emplace_back(pathname);
        roots_map.insert(std::pair(r->get_name(), r));
        return r;
    };

    fs::path normalized = path.lexically_normal();
    auto path_it = normalized.begin();
    fs::path current_path = *path_it;
    if (current_path == normalized) {
      // the path contains only the file: add "fictitious" root path "./"
      current_path = DEF_ROOT_PATH;
    } else {
      // skip the root, which we have already read
      ++path_it;
    }
    auto last_component = --normalized.end(); // component with the file name
    FwdRef<DirEntity> current_dir = dir_try_emplace(roots_map, current_path.native());
    while(path_it != last_component) {
      current_path /= *path_it;
      auto& subdir_map = dir_subdir_entity_map[current_dir];
      current_dir = dir_try_emplace(subdir_map, current_path.native());
      ++path_it;
    }
    FwdRef<FileEntity> file = *file_entities.emplace_back(path, std::forward<decltype(args)>(args)...);
    auto& files_map = dir_file_map[current_dir];
    auto [it, newly_created] = files_map.try_emplace(normalized.native(), file);
    if (not newly_created) {
      file_entities.erase(file);
      throw std::runtime_error(std::format("file '{}' was already added", normalized.native()));
    }
    return it->second;
  }

  std::ranges::view decltype(auto) get_roots() const noexcept {
    return std::views::values(roots_map);
  }

  inline const auto& get_types_view() const noexcept { return type_entities; }

  inline const auto& get_functions_view() const noexcept { return function_entities; }

  inline const auto& get_variables_view() const noexcept { return variable_entities; }

  inline const auto& get_enums_view() const noexcept { return enum_entities; }

  inline const auto& get_namespaces_view() const noexcept { return namespace_entities; }

  inline const auto& get_typedefs_view() const noexcept { return typedef_entities; }

  inline const auto& get_concepts_view() const noexcept { return concept_entities; }

  inline const auto& get_operators_view() const noexcept { return operator_entities; }

  inline const auto& get_macros_view() const noexcept { return macro_entities; }

  inline std::optional<FwdRef<GroupEntity>> get_group(std::string_view name) const {
    auto it = name_group_map.find(name);
    if (it != name_group_map.cend()) {
      return {it->second};
    } else {
        return {};
    }
  }

  children_view_t<GroupEntity> get_groups_view() const {
    return group_edges.get_view();
  }

  template<entity ParentT> requires container_of<ParentT, TypeEntity>
  children_view_t<TypeEntity> get_type_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, TypeEntity>(parent);
  }

  children_view_t<TypeEntity> get_type_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<TypeEntity> {
      if constexpr (not container_of<ParentT, TypeEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<TypeEntity>()));
      } else {
        return this->_get_children_view<ParentT, TypeEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, TypeEntity>
  children_view_t<FunctionEntity> get_function_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, FunctionEntity>(parent);
  }

  children_view_t<FunctionEntity> get_function_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<FunctionEntity> {
      if constexpr (not container_of<ParentT, FunctionEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<FunctionEntity>()));
      } else {
      return this->_get_children_view<ParentT, FunctionEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, VariableEntity>
  children_view_t<VariableEntity> get_variable_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, VariableEntity>(parent);
  }

  children_view_t<VariableEntity> get_variable_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<VariableEntity> {
      if constexpr (not container_of<ParentT, VariableEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<VariableEntity>()));
      } else {
        return this->_get_children_view<ParentT, VariableEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, EnumEntity>
  children_view_t<EnumEntity> get_enum_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, EnumEntity>(parent);
  }

  children_view_t<EnumEntity> get_enum_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<EnumEntity> {
      if constexpr (not container_of<ParentT, EnumEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<EnumEntity>()));
      } else {
      return this->_get_children_view<ParentT, EnumEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, NamespaceEntity>
  children_view_t<NamespaceEntity> get_namespace_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, NamespaceEntity>(parent);
  }

  children_view_t<NamespaceEntity> get_namespace_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<NamespaceEntity> {
      if constexpr (not container_of<ParentT, NamespaceEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<NamespaceEntity>()));
      } else {
        return this->_get_children_view<ParentT, NamespaceEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, TypedefEntity>
  children_view_t<TypedefEntity> get_typedef_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, TypedefEntity>(parent);
  }

  children_view_t<TypedefEntity> get_typedef_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<TypedefEntity> {
      if constexpr (not container_of<ParentT, TypedefEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<TypedefEntity>()));
      } else {
        return this->_get_children_view<ParentT, TypedefEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, ConceptEntity>
  children_view_t<ConceptEntity> get_concept_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, ConceptEntity>(parent);
  }

  children_view_t<ConceptEntity> get_concept_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<ConceptEntity> {
      if constexpr (not container_of<ParentT, ConceptEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<ConceptEntity>()));
      } else {
        return this->_get_children_view<ParentT, ConceptEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, OperatorEntity>
  children_view_t<OperatorEntity> get_operator_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, OperatorEntity>(parent);
  }

  children_view_t<OperatorEntity> get_operator_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<OperatorEntity> {
      if constexpr (not container_of<ParentT, OperatorEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<OperatorEntity>()));
      } else {
        return this->_get_children_view<ParentT, OperatorEntity>(p);
      }
    }, parent);
  }

  template<entity ParentT> requires container_of<ParentT, MacroEntity>
  children_view_t<MacroEntity> get_macro_children_for(const FwdRef<ParentT>& parent) const {
    return _get_children_view<ParentT, MacroEntity>(parent);
  }

  children_view_t<MacroEntity> get_macro_children_for(const RefVariant& parent) const {
    return std::visit([this]<entity ParentT>(const FwdRef<ParentT>& p) -> children_view_t<MacroEntity> {
      if constexpr (not container_of<ParentT, MacroEntity>) {
        throw std::logic_error(std::format("Entities of type {} have no children of type {}",
          entity_name<ParentT>(), entity_name<MacroEntity>()));
      } else {
        return this->_get_children_view<ParentT, MacroEntity>(p);
      }
    }, parent);
  }

private:
  struct file_catalog {
    std::optional<entities_storage<TypeEntity>::sequence_iter> type_opt;
    std::optional<entities_storage<FunctionEntity>::sequence_iter> function_opt;
    std::optional<entities_storage<VariableEntity>::sequence_iter> variable_opt;
    std::optional<entities_storage<EnumEntity>::sequence_iter> enum_opt;
    std::optional<entities_storage<NamespaceEntity>::sequence_iter> namespace_opt;
    std::optional<entities_storage<TypedefEntity>::sequence_iter> typedef_opt;
    std::optional<entities_storage<ConceptEntity>::sequence_iter> concept_opt;
    std::optional<entities_storage<OperatorEntity>::sequence_iter> operator_opt;
    std::optional<entities_storage<MacroEntity>::sequence_iter> macro_opt;

    file_catalog() = default;

    template<entity T> std::optional<typename entities_storage<T>::sequence_iter>& get_entity_opt() {
      if constexpr (std::same_as<T, TypeEntity>) {
        return type_opt;
      } else if constexpr (std::same_as<T, FunctionEntity>) {
        return function_opt;
      } else if constexpr (std::same_as<T, VariableEntity>) {
        return variable_opt;
      } else if constexpr (std::same_as<T, EnumEntity>) {
        return enum_opt;
      } else if constexpr (std::same_as<T, NamespaceEntity>) {
        return namespace_opt;
      } else if constexpr (std::same_as<T, TypedefEntity>) {
        return typedef_opt;
      } else if constexpr (std::same_as<T, ConceptEntity>) {
          return concept_opt;
      } else if constexpr (std::same_as<T, OperatorEntity>) {
          return operator_opt;
      } else /*if constexpr (std::same_as<T, MacroEntity>)*/ {
          return macro_opt;
      }
    }
  };

  mutable FwdList<RootEntity> root;

  // nodes
  FwdList<GroupEntity> group_entities;
  FwdList<TypeEntity> type_entities;
  FwdList<FunctionEntity> function_entities;
  FwdList<VariableEntity> variable_entities;
  FwdList<EnumEntity> enum_entities;
  FwdList<NamespaceEntity> namespace_entities;
  FwdList<TypedefEntity> typedef_entities;
  FwdList<ConceptEntity> concept_entities;
  FwdList<OperatorEntity> operator_entities;
  FwdList<MacroEntity> macro_entities;

  FwdList<FileEntity> file_entities;

  // edges
  entities_storage<GroupEntity> group_edges;
  edges_map_t<TypeEntity> type_edges;
  edges_map_t<FunctionEntity> function_edges;
  edges_map_t<VariableEntity> variable_edges;
  edges_map_t<EnumEntity> enum_edges;
  edges_map_t<NamespaceEntity> namespace_edges;
  edges_map_t<TypedefEntity> typedef_edges;
  edges_map_t<ConceptEntity> concept_edges;
  edges_map_t<OperatorEntity> operator_edges;
  edges_map_t<MacroEntity> macro_edges;


  std::unordered_map<std::string_view, file_catalog> name_root_map;
  std::unordered_map<std::string_view, FwdRef<GroupEntity>> name_group_map;
  // indexed by file first and then group name
  std::unordered_map<std::string_view, std::unordered_map<std::string_view, file_catalog>> filename_group_catalogs_map;



  static inline constexpr std::string_view DEF_ROOT_PATH = "./";

  FwdList<DirEntity> directories;
  subdir_map_t roots_map;
  std::unordered_map<FwdHashable, subdir_map_t> dir_subdir_entity_map;
  std::unordered_map<FwdHashable, std::unordered_map<std::string_view, FwdRef<FileEntity>>> dir_file_map;
};
