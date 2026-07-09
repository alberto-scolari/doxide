#pragma once

#include "Entities.hpp"
#include "FwdRef.hpp"

#include <concepts>

// helper that takes a runtime FwdRef<T> (where T satisfies entity) and returns the constexpr name
template<entity T> constexpr std::string_view entity_name(const FwdRef<T>&) {
  return entity_name<T>();
}

template<typename ContT, typename T> struct __entity_container {
  constexpr static bool value = false;
};

#define _DEFINE_CONTAINER_BASE(type_name, small_type_name) \
  class type_name##Container {}; \
  \
  template<typename ContT> concept small_type_name##s_container = (std::derived_from<ContT, type_name##Container> and entity<ContT>); \
  \
  template<entity ContT> struct __entity_container< ContT, type_name##Entity > { \
    constexpr static bool value = small_type_name##s_container< ContT >; \
  };


_DEFINE_CONTAINER_BASE(Type, type)

_DEFINE_CONTAINER_BASE(Function, function)

_DEFINE_CONTAINER_BASE(Variable, variable)

_DEFINE_CONTAINER_BASE(Enum, enum)

_DEFINE_CONTAINER_BASE(Namespace, namespace)

_DEFINE_CONTAINER_BASE(Typedef, typedef)

_DEFINE_CONTAINER_BASE(Concept, concept)

_DEFINE_CONTAINER_BASE(Operator, operator)

_DEFINE_CONTAINER_BASE(Macro, macro)

_DEFINE_CONTAINER_BASE(Group, group)

template<typename ContT, typename T> concept container_of =
  (__entity_container<ContT, T>::value) and entity<ContT> and entity<T>;
