#pragma once

#include <string_view>



/**
 * Query for entities in C++ sources.
 *
 * @ingroup developer
 */
constexpr std::string_view QUERY_CPP = R""""(
[
  ;; documentation
  (comment) @docs

  ;; namespace definition
  (namespace_definition
      name: (namespace_identifier) @name
      body: (declaration_list)? @body) @namespace

  ;; nested namespace definition---matches once for each @name
  (namespace_definition
      (nested_namespace_specifier) @nested_name
      body: (declaration_list)? @body) @namespace

  ;; template declaration
  (template_declaration
      [
        (class_specifier)
        (struct_specifier)
        (union_specifier)
        (alias_declaration)
        (concept_definition)
        (declaration)
        (field_declaration)
        (function_definition)
      ] @body) @template

  ;; class definition
  (class_specifier
      name: [
        (type_identifier) @name
        (template_type) @name  ;; for template specialization
      ]
      body: (field_declaration_list)? @body
      ) @type

  ;; struct definition
  (struct_specifier
      name: [
        (type_identifier) @name
        (template_type) @name  ;; for template specialization
      ]
      body: (field_declaration_list)? @body
      ) @type

  ;; union definition
  (union_specifier
      name: [
        (type_identifier) @name
        (template_type) @name  ;; for template specialization
      ]
      body: (field_declaration_list)? @body
      ) @type

  ;; enum definition
  (enum_specifier
      name: (type_identifier) @name
      body: (enumerator_list)? @body
      ) @type

  ;; typedef
  (type_definition
      declarator: [
        (type_identifier) @name

        ;; for function pointer types
        (function_declarator
          declarator: (parenthesized_declarator
            (pointer_declarator
              declarator: (type_identifier) @name)
          )
        )
        (pointer_declarator
          declarator: (function_declarator
            declarator: (type_identifier) @name)
        )
      ]) @typedef

  ;; type alias
  (alias_declaration
      name: (type_identifier) @name) @typedef

  ;; concept
  (concept_definition
      name: (identifier) @name
      (_)) @concept

  ;; variable
  (declaration
      declarator: [
        (identifier) @name
        (array_declarator (identifier) @name)
        (reference_declarator (identifier) @name)
        (pointer_declarator (identifier) @name)
        (init_declarator
          declarator: [
            (identifier) @name
            (array_declarator (identifier) @name)
            (reference_declarator (identifier) @name)
            (pointer_declarator (identifier) @name)
          ]
          value: (_) @value)

        ;; for function pointer types
        (function_declarator
          declarator: (parenthesized_declarator
            (pointer_declarator
              declarator: (identifier) @name)
          )
        )
      ]
      default_value: (_)? @value
    ) @variable

  ;; member variable
  (field_declaration
      declarator: [
        (field_identifier) @name
        (array_declarator (field_identifier) @name)
        (reference_declarator (field_identifier) @name)
        (pointer_declarator (field_identifier) @name)
        (init_declarator
          declarator: [
            (identifier) @name
            (field_identifier) @name
            (array_declarator (identifier) @name)
            (array_declarator (field_identifier) @name)
            (reference_declarator (identifier) @name)
            (reference_declarator (field_identifier) @name)
            (pointer_declarator (identifier) @name)
            (pointer_declarator (field_identifier) @name)
          ]
          value: (_) @value)

        ;; for function pointer types
        (function_declarator
          declarator: (parenthesized_declarator
            (pointer_declarator
              declarator: (field_identifier) @name)
          )
        )
      ]
      default_value: (_)? @value
    ) @variable

  ;; function declaration
  (declaration
      declarator: [
        (function_declarator
          declarator: (identifier) @name
        )
        (reference_declarator
          (function_declarator
            declarator: (identifier) @name
          )
        )
        (pointer_declarator
          (function_declarator
            declarator: (identifier) @name
          )
        )
        (function_declarator  ;; for function pointer return types
          declarator: (parenthesized_declarator
            (pointer_declarator
              (function_declarator
                declarator: (identifier) @name
              )
            )
          )
        )
      ]
    ) @function

  ;; member function declaration
  (field_declaration
      declarator: [
        (function_declarator
          declarator: [
            (identifier) @name
            (field_identifier) @name
          ]
        )
        (reference_declarator
          (function_declarator
            declarator: [
              (identifier) @name
              (field_identifier) @name
            ]
          )
        )
        (pointer_declarator
          (function_declarator
            declarator: [
              (identifier) @name
              (field_identifier) @name
            ]
          )
        )
        (function_declarator  ;; for function pointer return types
          declarator: (parenthesized_declarator
            (pointer_declarator
              (function_declarator
                declarator: [
                  (identifier) @name
                  (field_identifier) @name
                ]
              )
            )
          )
        )
      ]
    ) @function

  ;; function definition
  (function_definition
      declarator: [
        (function_declarator
          declarator: [
            (identifier) @name
            (field_identifier) @name
            (destructor_name) @name
          ]
        )
        (reference_declarator
          (function_declarator
            declarator: [
              (identifier) @name
              (field_identifier) @name
              (destructor_name) @name
            ]
          )
        )
        (pointer_declarator
          (function_declarator
            declarator: [
              (identifier) @name
              (field_identifier) @name
              (destructor_name) @name
            ]
          )
        )

        ;; for function pointer return types
        (function_declarator
          declarator: (parenthesized_declarator
            (pointer_declarator
              (function_declarator
                declarator: [
                  (identifier) @name
                  (field_identifier) @name
                  (destructor_name) @name
                ]
              )
            )
          )
        )
      ]
      [
        (field_initializer_list)
        body: (_)
      ] @body
    ) @function

  ;; constructor & destructor declaration
  (declaration
      declarator: [
        (function_declarator
          declarator: [
          (identifier) @name
          (field_identifier) @name
          (destructor_name) @name
          ]
        )
      ]
    ) @function

  ;; operator declaration
  (declaration
      declarator: [
        (function_declarator
          declarator: (operator_name) @name
        )
        (reference_declarator
          (function_declarator
            declarator: (operator_name) @name
          )
        )
        (pointer_declarator
          (function_declarator
            declarator: (operator_name) @name
          )
        )
        (operator_cast
          type: (_) @name
        )
      ]
    ) @operator

  ;; member operator declaration
  (field_declaration
      declarator: [
        (function_declarator
          declarator: (operator_name) @name
        )
        (reference_declarator
          (function_declarator
            declarator: (operator_name) @name
          )
        )
        (pointer_declarator
          (function_declarator
            declarator: (operator_name) @name
          )
        )
        (operator_cast
          type: (_) @name
        )
      ]
    ) @operator

  ;; member operator definition
  (function_definition
      declarator: [
        (function_declarator
          declarator: (operator_name) @name
        )
        (reference_declarator
          (function_declarator
            declarator: (operator_name) @name
          )
        )
        (pointer_declarator
          (function_declarator
            declarator: (operator_name) @name
          )
        )
        (operator_cast
          type: (_) @name
        )
      ]
      body: (_) @body
    ) @operator

  ;; enumeration value
  (enumerator
       name: (identifier) @name) @enumerator

  ;; macro
  (preproc_def
      name: (identifier) @name
      value: (_) @value) @macro
  (preproc_function_def
      name: (identifier) @name
      value: (_) @value) @macro
]
)"""";

/**
 * Query for entities to explicitly exclude from  line counts in C++ sources.
 *
 * @ingroup developer
 */
constexpr std::string_view QUERY_CPP_EXCLUDE = R""""(
[
  ;; types and parameter default values may contain expressions
  (_ type: (_) @exclude)
  (_ default_value: (_) @exclude)

  ;; template arguments in template specializations may contain expressions
  (class_specifier name: (_) @exclude)
  (struct_specifier name: (_) @exclude)
  (union_specifier name: (_) @exclude)

  ;; constexpr context
  (requires_clause) @exclude
  (static_assert_declaration) @exclude
  (type_definition) @exclude
  (alias_declaration) @exclude
  (concept_definition) @exclude
  (preproc_def) @exclude
  (preproc_function_def) @exclude
  (decltype) @exclude
  (sizeof_expression) @exclude
  (enum_specifier) @exclude

  ;; if statement may be if constexpr, special handling in code for this
  (if_statement condition: (_) @then_exclude) @if_constexpr
  (declaration (type_qualifier) @if_constexpr declarator: (_) @then_exclude)
]
)"""";

/**
 * Query for entities to explicitly include in line counts in C++ sources.
 *
 * @ingroup developer
 */
constexpr std::string_view QUERY_CPP_INCLUDE = R""""(
[
  (unary_expression operator: _ @executable)
  (binary_expression operator: _ @executable)
  (assignment_expression operator: _ @executable)
  (fold_expression operator: _ @executable)
  (field_expression operator: _ @executable)
  (co_await_expression operator: _ @executable)
  (new_expression) @executable
  (delete_expression) @executable
  (update_expression) @executable
  (subscript_expression) @executable
  (field_initializer) @executable
  (for_range_loop right: _ @executable)
  (return_statement) @executable
  (co_return_statement) @executable
  (co_yield_statement) @executable

  ;; function calls are complicated by higher-order functions, including
  ;; lambda use such as [](auto x){ return f(x); }(x); anchor on some simpler
  ;; cases
  (call_expression
    function: [
      (identifier)
      (qualified_identifier)
      (template_function)
    ] @executable)

]
)"""";
