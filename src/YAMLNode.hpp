#pragma once

#include "Utils.hpp"

#include <format>
#include <functional>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

/**
 * Node in a YAML document.
 *
 * @ingroup developer
 */
class YAMLNode {
public:
  using key_type = std::string;
  using key_view_type = std::string_view;
  using value_type = std::string;
  using sequence_type = std::list<std::unique_ptr<YAMLNode>>;
  using mapping_type = std::unordered_map<key_type, std::unique_ptr<YAMLNode>, het_string_hash, std::equal_to<>>;
  using variant_type = std::variant<value_type, sequence_type, mapping_type>;

  YAMLNode() = default;

  /**
   * Does this have a given key?
   */
  bool has(const key_view_type& key) const;

  /**
   * Is this a value?
   */
  bool isValue() const;

  /**
   * Queries the key, assuming that the current node is a mapping
   * (may throw otherwise). If the key dos not exist, it throws.
   *
   * This is quivalent to the std::unordered_map<>::at() method available
   * starting from C++26, where any type that is specified in the hasher
   * is accepted; hence, it allows querying via a string_view, although the
   * map internally stores a string. This, in turn, avoids implicit conversions
   * (and memory allocations) like in
   *
   * ```cpp
   * YAMLNode node;
   * // populate node
   * if (node.isSequence("groups")) { // implicit conversion and allocation
   *   const auto& seq = node.sequence("groups"); // here as well
   *   // ...
   * }
   * ```
   *
   * @param key Key.
   * @throw std::out_of_range in case the key does not exist
   */
  const std::unique_ptr<YAMLNode>& mapping_at(const key_view_type& key) const {
    const YAMLNode::mapping_type& m = mapping();
    auto it = m.find(key);
    if (it == m.cend()) {
      throw std::out_of_range(std::format("no key named \"{}\" in current YAML node", key));
    }
    return it->second;
  }

  /**
   * Is there a value at a given path?
   *
   * @param key Key.
   * @param keys... Keys.
   */
  template<class... Args>
  bool isValue(const key_view_type& key, Args&&... keys) const {
    return has(key) &&
        mapping_at(key)->isValue(std::forward<Args>(keys)...);
  }

  /**
   * Is this a sequence?
   */
  bool isSequence() const;

  /**
   * Is there a sequence at a given path?
   *
   * @param key Key.
   * @param keys... Keys.
   */
  template<class... Args>
  bool isSequence(const key_view_type& key, Args&&... keys) const {
    return has(key) &&
        mapping_at(key)->isSequence(std::forward<Args>(keys)...);
  }

  /**
   * Is this a mapping?
   */
  bool isMapping() const;

  /**
   * Is there a mapping at a given path?
   *
   * @param key Key.
   * @param keys... Keys.
   */
  template<class... Args>
  bool isMapping(const key_view_type& key, Args&&... keys) const {
    return has(key) &&
        mapping_at(key)->isMapping(std::forward<Args>(keys)...);
  }

  /**
   * Get this as a value.
   */
  const value_type& value() const;

  /**
   * Get the value at a given path.
   *
   * @param key Key.
   * @param keys... Keys.
   *
   * @return The value.
   */
  template<class... Args>
  const value_type& value(const key_view_type& key, Args&&... keys) const {
    return mapping_at(key)->value(std::forward<Args>(keys)...);
  }

  /**
   * Get this as a sequence.
   */
  sequence_type& sequence();

  /**
   * Get this as a sequence.
   */
  const sequence_type& sequence() const;

  /**
   * Get the sequence at a given path.
   *
   * @param key Key.
   * @param keys... Keys.
   *
   * @return The sequence.
   */
  template<class... Args>
  sequence_type& sequence(const key_view_type& key, Args&&... keys) {
    return mapping_at(key)->sequence(std::forward<Args>(keys)...);
  }

  /**
   * Get the sequence at a given path.
   *
   * @param key Key.
   * @param keys... Keys.
   *
   * @return The sequence.
   */
  template<class... Args>
  const sequence_type& sequence(const key_view_type& key, Args&&... keys) const {
    return mapping_at(key)->sequence(std::forward<Args>(keys)...);
  }

  /**
   * Get this as a mapping.
   */
  mapping_type& mapping();

  /**
   * Get this as a mapping.
   */
  const mapping_type& mapping() const;

  /**
   * Get the mapping at a given path.
   *
   * @param key Key.
   * @param keys... Keys.
   *
   * @return The mapping.
   */
  template<class... Args>
  mapping_type& mapping(const key_view_type& key, Args&&... keys) {
    return mapping_at(key)->mapping(std::forward<Args>(keys)...);
  }

  /**
   * Get the mapping at a given path.
   *
   * @param key Key.
   * @param keys... Keys.
   *
   * @return The mapping.
   */
  template<class... Args>
  const mapping_type& mapping(const key_view_type& key, Args&&... keys) const {
    return mapping_at(key)->mapping(std::forward<Args>(keys)...);
  }

  /**
   * Set the value.
   */
  void set(variant_type&& contents);

  /**
   * Set as a sequence.
   */
  void setSequence();

  /**
   * Set as a mapping.
   */
  void setMapping();

  /**
   * For a sequence, push an uninitialized node onto the end and return it.
   */
  YAMLNode& push();

  /**
   * For a mapping, insert a key with an uninitialized value and return the
   * value.
   */
  YAMLNode& insert(key_type&& key);

private:
  /**
   * Contents.
   */
  variant_type contents;
};
