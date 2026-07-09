#pragma once

#include <concepts>
#include <string_view>

class RootEntity;
class TypeEntity;
class FunctionEntity;
class VariableEntity;
class EnumEntity;
class NamespaceEntity;
class TypedefEntity;
class ConceptEntity;
class OperatorEntity;
class MacroEntity;
class GroupEntity;
class FileContentEntity;

template<typename T> concept entity = (std::same_as<T, TypeEntity>
  || std::same_as<T, FunctionEntity>
  || std::same_as<T, VariableEntity>
  || std::same_as<T, EnumEntity>
  || std::same_as<T, NamespaceEntity>
  || std::same_as<T, RootEntity>
  || std::same_as<T, TypedefEntity>
  || std::same_as<T, ConceptEntity>
  || std::same_as<T, OperatorEntity>
  || std::same_as<T, MacroEntity>
  || std::same_as<T, GroupEntity>
  || std::same_as<T, FileContentEntity>
);

template<typename> inline constexpr bool _always_false_v = false;

// constexpr mapping from entity type to human-readable name
template<entity T> constexpr std::string_view entity_name() {
  if constexpr (std::same_as<T, RootEntity>) return "RootEntity";
  else if constexpr (std::same_as<T, GroupEntity>) return "GroupEntity";
  else if constexpr (std::same_as<T, TypeEntity>) return "TypeEntity";
  else if constexpr (std::same_as<T, FunctionEntity>) return "FunctionEntity";
  else if constexpr (std::same_as<T, VariableEntity>) return "VariableEntity";
  else if constexpr (std::same_as<T, EnumEntity>) return "EnumEntity";
  else if constexpr (std::same_as<T, NamespaceEntity>) return "NamespaceEntity";
  else if constexpr (std::same_as<T, TypedefEntity>) return "TypedefEntity";
  else if constexpr (std::same_as<T, ConceptEntity>) return "ConceptEntity";
  else if constexpr (std::same_as<T, OperatorEntity>) return "OperatorEntity";
  else if constexpr (std::same_as<T, MacroEntity>) return "MacroEntity";
  else if constexpr (std::same_as<T, FileContentEntity>) return "FileContentEntity";
  else static_assert(_always_false_v<T>, "entity_name: unsupported entity type");
}
