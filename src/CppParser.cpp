#include "CppParser.hpp"

#include "Doc.hpp"
#include "Log.hpp"
#include "TextLineCursor.hpp"
#include "doxide.hpp"
#include "CppQueries.hpp"
#include "Regex.hpp"
#include "entities/EntityRegistry.hpp"
#include "entities/FileEntity.hpp"
#include "entities/RefVariant.hpp"

#include <algorithm>
#include <cctype>
#include <concepts>
#include <cstring>
#include <format>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <regex>
#include <stdexcept>
#include <string.h>
#include <unordered_map>
#include <utility>
#include <variant>

#include <generated/cpp_query_enums.hpp>

class CodeEntity;
class NamespaceEntity;
class RootEntity;
class TypeEntity;
class VariableEntity;

/**
 * Tree-sitter CUDA language handle.
 *
 * @ingroup developer
 */
extern "C" const TSLanguage* tree_sitter_cuda();

CppParser::CppParser() :
    parser(nullptr),
    query(nullptr),
    query_exclude(nullptr),
    query_include(nullptr) {
  uint32_t error_offset;
  TSQueryError error_type;

  /* parser */
  parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_cuda());

  /* queries */
  query = ts_query_new(tree_sitter_cuda(), QUERY_CPP.data(),
      uint32_t(QUERY_CPP.length()), &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    std::string_view from(QUERY_CPP.data() + error_offset,
        std::min(size_t(40), QUERY_CPP.length() - error_offset));
    error("invalid query starting '" << from << "'...");
  }

  query_exclude = ts_query_new(tree_sitter_cuda(), QUERY_CPP_EXCLUDE.data(),
      uint32_t(QUERY_CPP_EXCLUDE.length()), &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    std::string_view from(QUERY_CPP_EXCLUDE.data() + error_offset,
        std::min(size_t(40), QUERY_CPP_EXCLUDE.length() - error_offset));
    error("invalid query starting '" << from << "'...");
  }

  query_include = ts_query_new(tree_sitter_cuda(), QUERY_CPP_INCLUDE.data(),
      uint32_t(QUERY_CPP_INCLUDE.length()), &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    std::string_view from(QUERY_CPP_INCLUDE.data() + error_offset,
        std::min(size_t(40), QUERY_CPP_INCLUDE.length() - error_offset));
    error("invalid query starting '" << from << "'...");
  }
}

CppParser::~CppParser() {
  ts_query_delete(query);
  ts_query_delete(query_exclude);
  ts_query_delete(query_include);
  ts_parser_delete(parser);
}

static bool cannot_contain_subentities(const RefVariant& ref) {
  return ref.is_type<VariableEntity>();
}

std::stack<CppParser::StackEntry>::const_reference CppParser::pop_entities_downto_parent(uint32_t start, [[maybe_unused]] uint32_t end) {
  while(entities_stack.size() > 0 and
    (entities_stack.top().end < start or cannot_contain_subentities(entities_stack.top().ref)
    )
  ) {
    const bool is_child_visible = entities_stack.top().ref.is_visible();
    const bool child_has_no_group =  entities_stack.top().ref.get_group().empty();
    entities_stack.pop();
    if (entities_stack.size() > 0 and child_has_no_group) {
      entities_stack.top().ref.set_visible_child(is_child_visible);
    }
  }
  if (entities_stack.size() == 0) {
    throw std::runtime_error("no more elements in stack");
  }
  return entities_stack.top();
}

void CppParser::pop_all() {
  while(entities_stack.size() > 0) {
    const bool is_child_visible = entities_stack.top().ref.is_visible();
    const bool child_has_no_group =  entities_stack.top().ref.get_group().empty();
    entities_stack.pop();
    if (entities_stack.size() > 0 and child_has_no_group) {
      entities_stack.top().ref.set_visible_child(is_child_visible);
    }
  }
}

std::stack<CppParser::StackEntry>::reference CppParser::push(RefVariant ref, uint32_t start, uint32_t end) {
  return entities_stack.emplace(ref, start, end);
}

uint32_t get_decl_end(const TextLineCursor& str, uint32_t start, uint32_t middle) {
  while(middle > start && (
      std::isspace(str[middle - 1])
      or str[middle - 1] == '\\'
      or str[middle - 1] == '='
      or str[middle - 1] == ';'
    )
  ) {
    middle--;
  }
  return middle;
}

enum class MatchAction {
  NOTHING,
  ENTITY,
  NAMESPACE
};

struct DeclCoordinates {
  uint32_t start;
  uint32_t start_line;
  uint32_t end;
};

struct EntityState {
  uint32_t start = 0;
  uint32_t end = 0;
  uint32_t start_line = -1;
  uint32_t end_line = -1;
  uint32_t decl_end = -1;
  QueryCppNodes entity_type;
  bool is_nested = false;
  bool matched_template = false;
  std::optional<DeclCoordinates> template_decl;
  TextLineCursor decl_field;
  TextLineCursor name_field;

  void set_offsets(const TSNode& node);

  void update_decl_field(const TextLineCursor& file_content);

  void reset();
};

void EntityState::set_offsets(const TSNode& node) {
  start = ts_node_start_byte(node);
  start_line = ts_node_start_point(node).row;
  end = ts_node_end_byte(node);
  end_line = ts_node_end_point(node).row;
}

void EntityState::update_decl_field(const TextLineCursor& file_content) {
  uint32_t new_decl_end = get_decl_end(file_content, start, decl_end);
  decl_field = file_content.substr(start, new_decl_end - start);
}

void EntityState::reset() {
  is_nested = false;
  template_decl.reset();
  matched_template = false;
}

enum class CommentType {
  NONDOC,
  MULTI_FORWARD,
  MULTI_BACKWARD,
  SINGLE_CONTINUE,
  QT_SINGLE_CONTINUE,
  SINGLE_BACKWARD_OPEN,
  QT_SINGLE_BACKWARD_OPEN
};

struct CommentState {
  uint32_t begin = 0;
  uint32_t end = 0;
  uint32_t end_line = 0;
  CommentType type = CommentType::NONDOC;
};

struct ParserState {
  const TextLineCursor &file_content;
  const FwdRef<FileEntity>& file;
  const EntityRegistry& registry;
  TSQuery* query;
  EntityRegistry::EntityFileInserter& inserter;

  uint32_t node_start;
  uint32_t node_end;
  uint32_t start_line;
  std::optional<RefVariant> previous_match;
};

void check_group(Doc& docs, const ParserState& ps) {
  if (not docs.ingroup.empty() and not ps.registry.get_group(docs.ingroup.view())) {
    warn("file " << ps.file->get_path() << " line " << docs.ingroup.get_line_number() + 1 <<
        ": unrecognized group '" << docs.ingroup.view() <<
        "', groups must be defined in config file, ignoring @ingroup");
    docs.drop_group();
  }
}

void make_backward_docs(CommentState &doc_buffer, ParserState& ps) {
  constexpr uint32_t NOLINE = std::numeric_limits<uint32_t>::max();
  auto last_line_getter = [NOLINE]<entity T>(const FwdRef<T>& e) -> std::pair<uint32_t, uint32_t> {
    if constexpr (not std::derived_from<T, CodeEntity>) {
      return std::make_pair(NOLINE, NOLINE);
    } else {
      return std::make_pair(e->get_start_line(), e->get_end_line());
    }
  };
  auto [entity_first_line, entity_last_line] = ps.previous_match ? std::visit(last_line_getter, *ps.previous_match) :
   std::make_pair(NOLINE, NOLINE);

  if (doc_buffer.end_line < entity_last_line or doc_buffer.end_line > entity_last_line + 1) {
    return;
  }
  if (not ps.previous_match->get_docs().empty()) {
    warn("entity " << ps.previous_match->get_name() << " in file " << ps.file->get_path()
      << ", line " << entity_first_line << " - " << entity_last_line
      << " already has a comment, but a second backward one exists starting at line "
      << entity_last_line);
  }
  TextLineCursor comment = ps.file_content.substr(doc_buffer.begin, doc_buffer.end - doc_buffer.begin);
  Doc docs = Doc::from_comment(comment);
  check_group(docs, ps);
  ps.previous_match->set_docs(std::move(docs));
}

// from https://www.doxygen.nl/manual/docblocks.html
const std::regex comment_regex(
  R"(^\s*)" // beginning of string, than any whitespace (ignore)
  R"((?:)"  // start of non-capturing group
  R"((\/\*[\*|!])(?!<))" // opening comment /** or /*!
  R"(|)"
  R"((\/\/\/)(?!<))" // line continuation comment /// (without <)
  R"(|)"
  R"((\/\/!)(?!<))" // Qt line continuation comment //! (without <)
  R"(|)"
  R"((\/\*[\*|!]<))" // backward opening comment /**< or /*!<
  R"(|)"
  R"((\/\/\/<))" // backward-looking comment ///<
  R"(|)"
  R"((\/\/!<))" // Qt backward-looking comment //!<
  R"())", // end of non-capturing group
  REGEX_FLAGS);

CommentType get_comment_type(const TextLineCursor& text) {
  // TODO: avoid heap allocations with CTRE
  std::cmatch match;
  if (std::regex_search(text.cbegin(), text.cend(), match, comment_regex)) {
    if (match[1].matched) {
      return CommentType::MULTI_FORWARD;
    }
    else if (match[2].matched) {
      return CommentType::SINGLE_CONTINUE;
    }
    else if (match[3].matched) {
      return CommentType::QT_SINGLE_CONTINUE;
    }
    else if (match[4].matched) {
      return CommentType::MULTI_BACKWARD;
    }
    else if (match[5].matched) {
      return CommentType::SINGLE_BACKWARD_OPEN;
    }
    else if (match[6].matched) {
      return CommentType::QT_SINGLE_BACKWARD_OPEN;
    }
  }
  return CommentType::NONDOC;
}

void handle_comment(CommentState& doc_buffer, ParserState& ps) {
  CommentType type = get_comment_type(ps.file_content.substr(ps.node_start, ps.node_end - ps.node_start));

  if(doc_buffer.type != type // comments are of different format
    or ps.start_line > doc_buffer.end_line + 1 // or there is gap between comments
  ) {
    if(doc_buffer.type == CommentType::SINGLE_BACKWARD_OPEN
      or doc_buffer.type == CommentType::QT_SINGLE_BACKWARD_OPEN
    ) {
      // if the previous one is backward, flush it
      make_backward_docs(doc_buffer, ps);
    }
    // reset previous
    doc_buffer.type = CommentType::NONDOC;
  }

  switch (type) {
    case CommentType::NONDOC: {
      // reset
      doc_buffer = {0, 0, 0, CommentType::NONDOC};
      break;
    }
    case CommentType::MULTI_BACKWARD: {
      // process immediately
      CommentState current{ps.node_start, ps.node_end, ps.start_line, type};
      make_backward_docs(current, ps);
      doc_buffer.type = CommentType::NONDOC;
      break;
    }
    case CommentType::MULTI_FORWARD: {
      doc_buffer = {ps.node_start, ps.node_end, ps.start_line, CommentType::MULTI_FORWARD};
      break;
    }
    case CommentType::SINGLE_CONTINUE:
    case CommentType::QT_SINGLE_CONTINUE:
    case CommentType::SINGLE_BACKWARD_OPEN:
    case CommentType::QT_SINGLE_BACKWARD_OPEN: {
      if (doc_buffer.type == type) {
        doc_buffer.end = ps.node_end;
        doc_buffer.end_line = ps.start_line;
      } else {
        doc_buffer = {ps.node_start, ps.node_end, ps.start_line, type};
      }
      break;
    }
  }
}

void make_forward_docs(CommentState &doc_buffer, ParserState& ps, Doc& docs) {
  if (doc_buffer.type == CommentType::MULTI_FORWARD
    or doc_buffer.type == CommentType::SINGLE_CONTINUE
    or doc_buffer.type == CommentType::QT_SINGLE_CONTINUE) {
    auto text = ps.file_content.substr(doc_buffer.begin, doc_buffer.end - doc_buffer.begin);
    docs = Doc::from_comment(text);
    check_group(docs, ps);
    doc_buffer.type = CommentType::NONDOC;
  } else if(doc_buffer.type == CommentType::SINGLE_BACKWARD_OPEN
    or doc_buffer.type == CommentType::QT_SINGLE_BACKWARD_OPEN
  ) {
    // if the previous one is backward, flush it
    make_backward_docs(doc_buffer, ps);
    doc_buffer.type = CommentType::NONDOC;
  }
}

RefVariant make_entity(const EntityState &match_state, const RefVariant& parent, ParserState& ps, Doc& doc) {
  auto& ms = match_state;
  std::string_view entity_decl = ms.decl_field.view();
  const bool visible = not doc.docs.empty();
  std::optional<RefVariant> result_opt;
  QueryCppNodes id = ms.entity_type;
  auto name = ms.name_field.view();
  uint32_t start_line = ms.start_line;
  std::string_view template_decl;
  if (ms.template_decl) {
    std::size_t len = ms.template_decl->end - ms.template_decl->start;
    start_line = ms.template_decl->start_line;
    template_decl = std::string_view(ps.file_content.data() + ms.template_decl->start, len);
  }

  switch (id) {
    case QueryCppNodes::FUNCTION: {
        auto ref = ps.inserter.add_function(parent, template_decl, entity_decl,
          name, ps.file, start_line, ms.end_line, std::move(doc), visible);
        result_opt.emplace(ref);
        break;
    }
    case QueryCppNodes::TYPE: {
        auto ref = ps.inserter.add_type(parent, template_decl, entity_decl,
          name, ps.file, start_line, ms.end_line, std::move(doc), visible);
        result_opt.emplace(ref);
        break;
    }
    case QueryCppNodes::VARIABLE: {
      auto ref = ps.inserter.add_variable(parent, template_decl, entity_decl,
        name, ps.file, start_line, ms.end_line, std::move(doc), visible);
      result_opt.emplace(ref);
      break;
    }
    case QueryCppNodes::CONCEPT: {
      auto ref = ps.inserter.add_concept(parent, template_decl, entity_decl,
        name, ps.file, start_line, ms.end_line, std::move(doc), visible);
      result_opt.emplace(ref);
      break;
    }
    case QueryCppNodes::OPERATOR: {
      auto ref = ps.inserter.add_operator(parent, template_decl, entity_decl,
        name, ps.file, start_line, ms.end_line, std::move(doc), visible);
      result_opt.emplace(ref);
      break;
    }
    case QueryCppNodes::ENUMERATOR: {
      auto ref = ps.inserter.add_enum(parent, entity_decl,
        name, ps.file, start_line, ms.end_line, std::move(doc), visible);
      result_opt.emplace(ref);
      break;
    }
    case QueryCppNodes::MACRO: {
      auto ref = ps.inserter.add_macro(parent, entity_decl,
        name, ps.file, start_line, ms.end_line, std::move(doc), visible);
      result_opt.emplace(ref);
      break;
    }
    case QueryCppNodes::TYPEDEF: {
      auto ref = ps.inserter.add_typedef(parent, template_decl, entity_decl,
        name, ps.file, start_line, ms.end_line, std::move(doc), visible);
      result_opt.emplace(ref);
      break;
    }
    default: {
      uint32_t len = 0;
      const char* name = ts_query_capture_name_for_id(ps.query, static_cast<uint32_t>(id), &len);
      throw std::runtime_error(std::format("Error: unhandeld entity of type {}", name));
      break;
    }
  }
  return *result_opt;
}

static constexpr std::string_view NAMESPACE_SEPARATOR{"::"};

std::pair<RefVariant, RefVariant> make_namespace(
  const EntityState &match_state,
  const RefVariant& parent,
  ParserState& ps,
  Doc& doc
) {
  auto& ms = match_state;
  bool visible = not doc.docs.empty();
  if (not ms.is_nested) {
    RefVariant first = ps.inserter.add_namespace(parent, ms.decl_field.view(), ms.name_field.view(), ps.file, ms.start_line, ms.end_line, std::move(doc), visible);
    return std::make_pair(first, first);
  }
  std::optional<FwdRef<NamespaceEntity>> first_opt;
  std::optional<FwdRef<NamespaceEntity>> previous_opt;
  auto parts = ms.name_field.view() | std::views::split(NAMESPACE_SEPARATOR);
  Doc empty_doc;
  const std::size_t name_offset = ms.name_field.data() - ms.decl_field.data();
  std::size_t current_name_offset = 0;
  for (auto it = parts.begin(); it != parts.end(); ++it) {
    std::size_t name_len = std::ranges::distance(*it);
    std::string_view name{ms.name_field.data() + current_name_offset, name_len};
    std::string_view decl{ms.decl_field.data(), name_offset + current_name_offset + name_len};
    current_name_offset += name_len + NAMESPACE_SEPARATOR.size(); // account for "::" separator
    const bool is_last = std::next(it) == parts.end();
    if (not is_last) {
      empty_doc = Doc();
    }
    Doc& d = not is_last? empty_doc : doc;
    auto entity = previous_opt ?
      ps.inserter.add_namespace(*previous_opt, decl, name, ps.file, ms.start_line, ms.end_line, std::move(d), visible) :
      ps.inserter.add_namespace(parent, decl, name, ps.file, ms.start_line, ms.end_line, std::move(d), visible);
    if (not first_opt) {
      // store the first ref to return
      first_opt = entity;
    }
    previous_opt = entity;
  }
  return std::make_pair(*first_opt, *previous_opt);
}

void CppParser::parse(
  EntityRegistry& registry,
  const std::filesystem::path& filename,
  const std::unordered_map<std::string, std::string>& defines
) {
  std::string _content = preprocess(filename, defines);
  TSTree* tree = ts_parser_parse_string(parser, NULL, _content.data(), static_cast<uint32_t>(_content.size()));
  if (!tree) {
    /* something went very wrong */
    warn("cannot parse " << filename << ", skipping");
    return;
  } else {
    /* report on any remaining parse errors */
    report(filename, _content, tree);
  }

  TSNode node = ts_tree_root_node(tree);
  FwdRef<FileEntity> file_ref = registry.add_file(filename,
    std::move(_content), ts_node_end_point(node).row + 1);
  FwdRef<RootEntity> root_ref = registry.get_root_ref();
  EntityRegistry::EntityFileInserter inserter = registry.get_inserter(file_ref);

  push(root_ref, 0, ts_node_end_byte(node));

  const TextLineCursor file_content(file_ref->get_content());

  using unique_cursor_ptr = std::unique_ptr<TSQueryCursor, decltype(&ts_query_cursor_delete)>;
  unique_cursor_ptr cursor(ts_query_cursor_new(), &ts_query_cursor_delete);
  ts_query_cursor_exec(cursor.get(), query, node);
  TSQueryMatch match;
  EntityState match_state;
  ParserState parser_state = {file_content, file_ref, registry, query, inserter, 0, 0, 0, {}};
  CommentState doc_buffer;

  while (ts_query_cursor_next_match(cursor.get(), &match)) {
    MatchAction action{MatchAction::NOTHING};

    for (uint16_t i = 0; i < match.capture_count; ++i) {
      node = match.captures[i].node;
      uint32_t id = match.captures[i].index;
      parser_state.node_start = ts_node_start_byte(node);
      parser_state.node_end = ts_node_end_byte(node);
      parser_state.start_line = ts_node_start_point(node).row;
      uint32_t node_len = parser_state.node_end - parser_state.node_start;
      // std::cout << "=== MATCHED " << to_string(static_cast<QueryCppNodes>(id)) << std::endl;

      switch (static_cast<QueryCppNodes>(id)) {
        case QueryCppNodes::DOCS: {
          handle_comment(doc_buffer, parser_state);
          break;
        }
        case QueryCppNodes::VALUE: {
          break;
        }
        case QueryCppNodes::NAME: {
          match_state.name_field = file_content.substr(parser_state.node_start, node_len);
          match_state.decl_end = parser_state.node_end;
          break;
        }
        case QueryCppNodes::NESTED_NAME: {
          match_state.name_field = file_content.substr(parser_state.node_start, node_len);
          match_state.decl_end = parser_state.node_end;
          match_state.is_nested = true;
        }
        case QueryCppNodes::BODY: {
          if (match_state.matched_template) {
            match_state.template_decl->end = get_decl_end(file_content, match_state.template_decl->start, parser_state.node_start);
            // done with template matching
            match_state.matched_template = false;
          } else {
            match_state.decl_end = parser_state.node_start;
          }
          break;
        }
        case QueryCppNodes::CONCEPT:
        case QueryCppNodes::OPERATOR:
        case QueryCppNodes::ENUMERATOR:
        case QueryCppNodes::MACRO:
        case QueryCppNodes::TYPEDEF:
        case QueryCppNodes::VARIABLE:
        case QueryCppNodes::FUNCTION:
        case QueryCppNodes::TYPE: {
          action = MatchAction::ENTITY;
          match_state.set_offsets(node);
          match_state.entity_type = static_cast<QueryCppNodes>(id);
          break;
        }
        case QueryCppNodes::TEMPLATE: {
          match_state.template_decl = {parser_state.node_start, ts_node_start_point(node).row, parser_state.node_end };
          match_state.matched_template = true;
          break;
        }
        case QueryCppNodes::NAMESPACE: {
          match_state.set_offsets(node);
          action = MatchAction::NAMESPACE;
          break;
        }
      }
    }

    switch (action) {
      case MatchAction::NOTHING: {
        break;
      }
      case MatchAction::ENTITY: {
        Doc docs;
        make_forward_docs(doc_buffer, parser_state, docs);
        match_state.update_decl_field(file_content);
        RefVariant parent = pop_entities_downto_parent(match_state.start, match_state.end).ref;
        if (parent.is_type<TypeEntity>()) {
          docs.drop_group();
        }
        if (not docs.ingroup.empty()) {
          auto group = registry.get_group(docs.ingroup);
          if (not group) {
            throw std::logic_error(std::format("group \"{}\" not found in internal registry", docs.ingroup.view()));
          }
          parent = *group;
        }
        auto child = make_entity(match_state, parent, parser_state, docs);
        parser_state.previous_match = child;
        std::format_to(std::ostream_iterator<char>(std::cout), ">> entity created under {}: {:?}\n", parent, child);
        // push child on top of the stack, based on the beginning and end of the entity
        push(child, match_state.start, match_state.end);
        match_state.reset();
        docs = Doc();
        break;
      }
      case MatchAction::NAMESPACE: {
        Doc docs;
        make_forward_docs(doc_buffer, parser_state, docs);
        if (not docs.ingroup.empty()) {
          warn("file " << filename.native() << " line " << docs.ingroup.get_line_number() + 1
            << ": namespace cannot have @ingroup, ignoring");
          docs.drop_group();
        }
        match_state.update_decl_field(file_content);
        const RefVariant& parent_entry = pop_entities_downto_parent(match_state.start, match_state.end).ref;
        auto [first_ns, last_ns] = make_namespace(match_state, parent_entry, parser_state, docs);
        parser_state.previous_match = last_ns;
        const std::string_view nested = match_state.is_nested ? "nested " : "";
        std::format_to(std::ostream_iterator<char>(std::cout), ">> {}namespace created under {}: {:?}\n", nested, parent_entry, last_ns);
        // push only the last namespace on the stack, since they are all in the same matching range
        push(last_ns, match_state.start, match_state.end);
        match_state.reset();
        docs = Doc();
        break;
      }
    }
  }
  // flush existing comments in buffer
  if (doc_buffer.type == CommentType::SINGLE_BACKWARD_OPEN
    or doc_buffer.type == CommentType::QT_SINGLE_BACKWARD_OPEN
  ) {
    make_backward_docs(doc_buffer, parser_state);
  }

  // pop remaining entries to apply the child's visibility and empty the stack
  pop_all();

  // sorted map of END, START, for optimized lookup of lower_bound (result >= query)
  std::map<uint32_t,uint32_t> excluded;
  bool constexpr_context = false;
  static const std::regex regex_if_constexpr("^(?:if\\s+)?constexpr", REGEX_FLAGS);
  cursor = unique_cursor_ptr(ts_query_cursor_new(), &ts_query_cursor_delete);
  node = ts_tree_root_node(tree);
  ts_query_cursor_exec(cursor.get(), query_exclude, node);
  while (ts_query_cursor_next_match(cursor.get(), &match)) {
    for (uint16_t i = 0; i < match.capture_count; ++i) {
      node = match.captures[i].node;
      uint32_t id = match.captures[i].index;
      uint32_t start = ts_node_start_byte(node);
      uint32_t end = ts_node_end_byte(node);
      switch (static_cast<QueryCppExcludeNodes>(id)) {
        case QueryCppExcludeNodes::EXCLUDE: {
          /* exclude any expressions in this region for line data */
          excluded.emplace(start, end);
          break;
        }
        case QueryCppExcludeNodes::THEN_EXCLUDE: {
          /* to be excluded if the last constexpr check was positive */
          if (constexpr_context) {
            excluded.emplace(start, end);
            constexpr_context = false;
          }
          break;
        }
        case QueryCppExcludeNodes::IF_CONSTEXPR: {
          /* check if this is `constexpr`, which is not reflected in the
          * parse tree and requires a string comparison */
          std::string_view stmt = file_content.substr(start, end - start);
          constexpr_context = std::regex_search(stmt.cbegin(), stmt.cend(), regex_if_constexpr);
          break;
        }
      }
    }
  }

  cursor = unique_cursor_ptr(ts_query_cursor_new(), &ts_query_cursor_delete);
  node = ts_tree_root_node(tree);
  ts_query_cursor_exec(cursor.get(), query_include, node);
  auto& line_counts = file_ref->get_line_counts();
  while (ts_query_cursor_next_match(cursor.get(), &match)) {
    for (uint16_t i = 0; i < match.capture_count; ++i) {
      node = match.captures[i].node;
      uint32_t id = match.captures[i].index;
      uint32_t start = ts_node_start_byte(node);
      uint32_t end = ts_node_end_byte(node);
      switch (static_cast<QueryCppIncludeNodes>(id)) {
        case QueryCppIncludeNodes::EXECUTABLE: {
          /* executable code, update line data as long as the code is not
            * within an excluded region */
          auto it = excluded.lower_bound(end);
          bool exclude = it != excluded.end() and it->first <= start;
          if (not exclude) {
            uint32_t start_line = ts_node_start_point(node).row;
            uint32_t end_line = ts_node_end_point(node).row;
            for (uint32_t line = start_line; line <= end_line; ++line) {
              if (line_counts[line] < 0) {
                line_counts[line] = 0;
                file_ref->get_included_lines()++;
              }
            }
          }
          break;
        }
      }
    }
  }
}

std::string CppParser::preprocess(const std::filesystem::path& filename,
    const std::unordered_map<std::string,std::string>& defines) {
  std::string in = gulp(filename);
  TSTree* tree = ts_parser_parse_string(parser, NULL, in.data(),
      uint32_t(in.size()));
  TSNode root = ts_tree_root_node(tree);
  TSNode node = root;
  TSTreeCursor cursor = ts_tree_cursor_new(root);
  do {
    uint32_t k = ts_node_start_byte(node);
    uint32_t l = ts_node_end_byte(node);
    TSPoint from = ts_node_start_point(node);
    bool nextNodeChosen = false;

    if (defines.contains(in.substr(k, l - k))) {
      /* replace preprocessor macro */
      const std::string& value = defines.at(in.substr(k, l - k));
      uint32_t old_size = uint32_t(in.size());
      in.replace(k, l - k, value);
      uint32_t new_size = uint32_t(in.size());
      TSPoint root_to = ts_node_end_point(root);

      /* update tree */
      TSInputEdit edit{k, old_size, new_size, from, root_to, root_to};
      ts_tree_edit(tree, &edit);
      ts_parser_reset(parser);
      TSTree* old_tree = tree;
      tree = ts_parser_parse_string(parser, old_tree, in.data(),
          uint32_t(in.size()));
      ts_tree_delete(old_tree);
      root = ts_tree_root_node(tree);

      /* restore cursor to same byte position as edit */
      ts_tree_cursor_reset(&cursor, root);
      while (ts_tree_cursor_goto_first_child_for_byte(&cursor, k) >= 0);

      /* next iteration from this position */
      nextNodeChosen = true;
    }

    /* next node */
    if (!nextNodeChosen) {
      if (strcmp(ts_node_type(node), "preproc_def") != 0 &&
          strcmp(ts_node_type(node), "preproc_function_def") != 0 &&
          ts_tree_cursor_goto_first_child(&cursor)) {
        // ^ do not recurse into preprocessor definitions, as we do not want
        //   to replace preprocessor macros there
      } else if (ts_tree_cursor_goto_next_sibling(&cursor)) {
        //
      } else while (ts_tree_cursor_goto_parent(&cursor) &&
          !ts_tree_cursor_goto_next_sibling(&cursor)) {
        //
      }
    }
    node = ts_tree_cursor_current_node(&cursor);
  } while (!ts_node_eq(node, root));

  ts_tree_cursor_delete(&cursor);
  ts_tree_delete(tree);
  ts_parser_reset(parser);
  return in;
}

void CppParser::report(const std::filesystem::path& filename,
    const std::string_view in, TSTree* tree) {
  TSNode root = ts_tree_root_node(tree);
  TSNode node = root;
  TSTreeCursor cursor = ts_tree_cursor_new(root);
  do {
    uint32_t k = ts_node_start_byte(node);
    uint32_t l = ts_node_end_byte(node);
    TSPoint from = ts_node_start_point(node);
    if (ts_node_is_error(node)) {
      warn(filename << ':' << (from.row + 1) << ':' << from.column <<
          ": warning: parse error at '" <<
          in.substr(k, std::min(l - k, 40u)) <<
          "', but will continue");
    }

    /* next node */
    if (strcmp(ts_node_type(node), "preproc_def") != 0 &&
        strcmp(ts_node_type(node), "preproc_function_def") != 0 &&
        ts_tree_cursor_goto_first_child(&cursor)) {
      // ^ do not recurse into preprocessor definitions
    } else if (ts_tree_cursor_goto_next_sibling(&cursor)) {
      //
    } else while (ts_tree_cursor_goto_parent(&cursor) &&
        !ts_tree_cursor_goto_next_sibling(&cursor)) {
      //
    }
    node = ts_tree_cursor_current_node(&cursor);
  } while (!ts_node_eq(node, root));
  ts_tree_cursor_delete(&cursor);
}
