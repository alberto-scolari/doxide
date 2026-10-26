#include "MarkdownGenerator.hpp"

#include <Log.hpp>
#include <FileSystemInfo.hpp>
#include <Sink.hpp>
#include <Utils.hpp>
#include <YAMLParser.hpp>
#include <YAMLNode.hpp>
#include <entities/ContainerBases.hpp>
#include <entities/Entities.hpp>
#include <entities/EntityRegistry.hpp>
#include <entities/FwdRef.hpp>
#include <entities/NamespaceEntity.hpp>
#include <entities/RefVariant.hpp>
#include <entities/TypeEntity.hpp>

#include "Dfs_traversal.hpp"
#include "EntityIterStack.hpp"
#include "MdCommon.hpp"
#include "Sanitize.hpp"

#include <algorithm>
#include <concepts>
#include <fstream>
#include <ranges>
#include <string_view>
#include <variant>
#include <vector>
#include <tuple>
#include <type_traits>
#include <utility>

class ConceptEntity;
class Documentable;
class FunctionEntity;
class GroupEntity;
class MacroEntity;
class OperatorEntity;
class RootEntity;
class Titled;
class TypedefEntity;
class VariableEntity;

// checks that R is a range whose double de-ref value is an entity
template<typename R> concept entity_range = requires (R r) {
  requires std::ranges::range<R>; // it's a range
  *std::ranges::begin(r); // the iterator points to a reference type (like FwdRef)
  typename std::remove_cvref_t<decltype(*std::ranges::begin(r))>::value_type; // the ref-box must have a value_type typedef (like FwdRef)
  requires entity<std::remove_cvref_t<
    typename std::remove_cvref_t<decltype(*std::ranges::begin(r))>::value_type>
    >; // if you de-ref, it must provide an FwdRef whose value_type is an entity
};

template<entity_range R> using entity_range_value_t = std::remove_cvref_t<decltype(std::declval<std::ranges::range_value_t<R>>())>;

template<entity_range R> static auto get_visible_view(R&& entities) {
  return entities | std::views::filter([](const auto& el) { return el->is_visible(); });
}
static auto name_getter = []<entity T>(const FwdRef<T>& e) {
  return e->get_name();
};

template<typename R>
  requires (std::ranges::random_access_range<R> and entity_range<R>)
  static void sort_by_name(R&& entities) {
  std::ranges::sort(entities, {}, name_getter);
  return;
}

template<entity U> constexpr std::string_view get_title(const FwdRef<U>& e) {
  if constexpr (std::derived_from<U, Titled>) {
    return e->get_title();
  } else {
    return e->get_name();
  }
}

template<entity_range R> void print_md_section_link(sink_t& sink, R &&entities,
  std::string_view section_name) {
  for (const auto&  e: entities) {
    sink.println(":{}: [{}]({}/)\n:   {}", section_name, get_title(e),
      sanitize(e->get_name()), line(brief(e)));
  }
}

void print_md_table_file_link(sink_t& sink, entity_range auto&& entities, std::string_view caption) {
  if (std::ranges::empty(entities)) {
    return;
  }
  sink.println("## {}\n", caption);
  sink.println("| Name | Description |");
  sink.println("| ---- | ----------- |");
  for (const auto& e: entities) {
    sink.println("| [{}]({}/) | {} |", e->get_name(), sanitize(e->get_name()), line(brief(e)));
  }
  sink.println();
}

void print_md_table(sink_t& sink, entity_range auto &&entities, std::string_view caption) {
  if (std::ranges::empty(entities)) {
    return;
  }
  sink.println("## {}\n", caption);
  sink.println("| Name | Description |");
  sink.println("| ---- | ----------- |");
  for (const auto& e: entities) {
    sink.println("| [{}](#{}) | {} |", e->get_name(), sanitize(e->get_name()), line(brief(e)));
  }
  sink.println();
}

void print_html_details(sink_t& sink, entity_range auto &&entities, std::string_view caption, std::string_view name) {
  if (std::ranges::empty(entities)) {
    return;
  }
  sink.println("## {} Details\n", caption);
  for (const auto& e: entities) {
    sink.println("### {}<a name=\"{}\"></a>\n", e->get_name(), sanitize(e->get_name()));
    sink.println("!!! {} \"{}\"\n", name, htmlize(line(e->get_decl())));
    sink.println("{}\n", indent(e->get_docs()));
  }
}

template<bool enabled, typename ValueT> struct vector_storage {};

template<typename ValueT> struct vector_storage< true, ValueT> {

  using StorageT = std::vector<ValueT>;

  StorageT& operator=(StorageT&& v) {
    storage = std::move(v);
    return storage;
  }

  template<std::ranges::range R> StorageT& operator=(R&& v) {
    storage.assign(std::ranges::begin(v), std::ranges::end(v));
    return storage;
  }

  StorageT& operator*() { return storage; }

  StorageT* operator->() { return &storage; }

  StorageT storage;
};

template<entity T> void render_entity_ref(
  sink_t& sink, const EntityRegistry& registry, const FwdRef<T>& root,
  const std::optional<std::string_view>& coverage_directory) {

  constexpr bool sort_entities = std::same_as<T, NamespaceEntity> or std::same_as<T, GroupEntity>;

  sink.print(FRONTMATTER);
  sink.println("\n# {}\n", get_title(root));

  if constexpr (std::is_same_v<T, TypeEntity>) {
    sink.println("**{}**\n", htmlize(line(root->get_decl())));
  }
  sink.println("{}\n", root->get_docs());

  if constexpr (groups_container<T>) {
    // anybody can see groups, but only Root is tagged as GroupsContainer
    auto groups = get_visible_view(registry.get_groups_view());
    print_md_section_link(sink, groups, "material-format-section");
  }

  if constexpr (namespaces_container<T>) {
    auto namespaces = get_visible_view(registry.get_namespace_children_for(root));
    std::vector<FwdRef<NamespaceEntity>> vec{std::ranges::begin(namespaces), std::ranges::end(namespaces)};
    sort_by_name(vec);
    print_md_section_link(sink, vec, "material-package");
  }

  if constexpr (std::same_as<T, RootEntity>) {
    if (coverage_directory) {
      sink.println(":material-chart-pie: [Code Coverage]({}/)\n:    \n", *coverage_directory);
    }
  }

  if constexpr (types_container<T>) {
    auto types = get_visible_view(registry.get_type_children_for(root));
    if (not sort_entities) {
      print_md_table_file_link(sink, types, "Types");
    } else {
      std::vector<FwdRef<TypeEntity>> vec{std::ranges::begin(types), std::ranges::end(types)};
      sort_by_name(vec);
      print_md_table_file_link(sink, vec, "Types");
    }
  }

  vector_storage<typedefs_container<T>, FwdRef<TypedefEntity>> typedef_vec;
  if constexpr (typedefs_container<T>) {
    typedef_vec = get_visible_view(registry.get_typedef_children_for(root));
    if (sort_entities) { sort_by_name(*typedef_vec); }
    print_md_table(sink, *typedef_vec, "Type Aliases");
  }

  vector_storage<concepts_container<T>, FwdRef<ConceptEntity>> concept_vec;
  if constexpr (concepts_container<T>) {
    concept_vec = get_visible_view(registry.get_concept_children_for(root));
    if (sort_entities) { sort_by_name(*concept_vec); }
    print_md_table(sink, *concept_vec, "Concepts");
  }

  vector_storage<macros_container<T>, FwdRef<MacroEntity>> macro_vec;
  if constexpr (macros_container<T>) {
    macro_vec = get_visible_view(registry.get_macro_children_for(root));
    if (sort_entities) { sort_by_name(*macro_vec); }
    print_md_table(sink, *macro_vec, "Macros");
  }

  vector_storage<variables_container<T>, FwdRef<VariableEntity>> variable_vec;
  if constexpr (variables_container<T>) {
    variable_vec = get_visible_view(registry.get_variable_children_for(root));
    if (sort_entities) { sort_by_name(*variable_vec); }
    print_md_table(sink, *variable_vec, "Variables");
  }

  vector_storage<operators_container<T>, FwdRef<OperatorEntity>> operator_vec;
  if constexpr (operators_container<T>) {
    operator_vec = get_visible_view(registry.get_operator_children_for(root));
    if (sort_entities) { sort_by_name(*operator_vec); }
    print_md_table(sink, *operator_vec, "Operators");
  }

  vector_storage<variables_container<T>, FwdRef<FunctionEntity>> function_vec;
  if constexpr (functions_container<T>) {
    function_vec = get_visible_view(registry.get_function_children_for(root));
    if (sort_entities) { sort_by_name(*function_vec); }
    print_md_table(sink, *function_vec, "Functions");
  }

  if constexpr (enums_container<T>) {
    auto enums = get_visible_view(registry.get_enum_children_for(root));
    if (not std::ranges::empty(enums)) {
      for (const auto& e: enums) {
        sink.println("**{}**\n:   {}\n", e->get_decl(), e->get_docs());
      }
    }
  }

  if constexpr (typedefs_container<T>) {
    if (not std::ranges::empty(*typedef_vec)) {
      if (not sort_entities) { sort_by_name(*typedef_vec); }
      print_html_details(sink, *typedef_vec, "Type Alias", "typedef");
    }
  }

  if constexpr (concepts_container<T>) {
    if (not std::ranges::empty(*concept_vec)) {
      if (not sort_entities) { sort_by_name(*concept_vec); }
      print_html_details(sink, *concept_vec, "Concept", "concept");
    }
  }

  if constexpr (macros_container<T>) {
    if (not std::ranges::empty(*macro_vec)) {
      if (not sort_entities) { sort_by_name(*macro_vec); }
      print_html_details(sink, *macro_vec, "Macros", "macro");
    }
  }

  if constexpr (variables_container<T>) {
    if (not std::ranges::empty(*variable_vec)) {
      if (not sort_entities) { sort_by_name(*variable_vec); }
      print_html_details(sink, *variable_vec, "Variable", "variable");
    }
  }

  if constexpr (operators_container<T>) {
    if (not std::ranges::empty(*operator_vec)) {
      if (not sort_entities) { sort_by_name(*operator_vec); }
      // cope with multiple declarations of an operator:
      // move entities with the same name at the bottom and remove them
      auto [first, last] = std::ranges::unique(*operator_vec, {}, name_getter);
      operator_vec->erase(first, last);
      print_html_details(sink, *operator_vec, "Operator", "operator");
    }
  }

  if constexpr (functions_container<T>) {
    if (not std::ranges::empty(*function_vec)) {
      if (not sort_entities) { sort_by_name(*function_vec); }
      auto [first, last] = std::ranges::unique(*function_vec, {}, name_getter);
      function_vec->erase(first, last);
      print_html_details(sink, *function_vec, "Function", "function");
    }
  }
}

void MarkdownGenerator::render_entity(const EntityRegistry& registry, const std::filesystem::path& output, const RefVariant& root, const std::string_view& sanitized_name, const std::optional<std::string_view>& coverage_directory) {
  constexpr std::string_view filename = "index.md";
  const std::string_view dirname = root.is_type<RootEntity>() ? "" : sanitized_name;
  sink_t csink;
  csink.println("\tdirname \"{}\", filename \"{}\", output \"{}\"", dirname, filename, output.native());
  std::filesystem::path file = output;
  if (dirname.size() > 0) {
    file /= dirname;
  }
  std::filesystem::create_directories(file);
  file /= filename;

  if (not can_write(file)) {
    return;
  }
  fscleaner.add_file(file);
  std::ofstream out(file);
  sink_t sink(out);

  std::visit([&]<typename T>(const FwdRef<T>& ref) {
    render_entity_ref(sink, registry, ref, coverage_directory);
  }, root);
}

void MarkdownGenerator::generate(const EntityRegistry& registry,
  const std::optional<std::string_view>& coverage_directory) {
  sink_t sink;
  EntityStackController controller{registry, output};
  for (const auto& [v, sanitized_name, output_path] : traverse_dfs_preorder(controller)) {
    sink.println("- getting entity: {} - \"{}\" - [sanitized: '{}'] on path {}",
      v.get_entity_name(), v.get_name(), sanitized_name, output_path.native());
    if (v.is_visible()) {
      render_entity(registry, output_path, v, sanitized_name, coverage_directory);
    }
  }
}
