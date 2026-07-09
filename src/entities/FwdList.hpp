#pragma once

#include "FwdRef.hpp"

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

template<typename T> class FwdListNode {
public:
  FwdListNode() = delete;

  template<typename U> friend class FwdList;
  template<typename U> friend class FwdListIterator;

private:
  FwdListNode(const FwdListNode& other) = delete;

  FwdListNode& operator=(const FwdListNode& other) = delete;

  template<typename... Args>
  constexpr explicit FwdListNode(Args&&... args)
    : value(std::forward<Args>(args)...), next(nullptr), prev(nullptr) {}

  T value;
  FwdListNode* next;
  FwdListNode* prev;

  static const FwdListNode* node_from_value(const T* value) {
    const std::size_t offset = reinterpret_cast<std::size_t>(&(reinterpret_cast<FwdListNode*>(0L)->value));
    return reinterpret_cast<const FwdListNode<T>*>(reinterpret_cast<std::size_t>(value) - offset);
  }
};

template<typename T> class FwdListSentinel{};

template<typename T> class FwdListIterator {
public:
  using value_type = T;
  using reference = FwdRef<T>; // return the wrapped pointer
  using const_reference = const FwdRef<T>; // return the wrapped pointer
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  constexpr FwdListIterator(): node_(nullptr) {}

  constexpr FwdListIterator(const FwdListIterator<T>& other) noexcept = default;

  constexpr FwdListIterator<T>& operator=(const FwdListIterator<T>& other) noexcept = default;

  constexpr reference operator*() noexcept {
    return static_cast<FwdRef<T>>(*this);
  }

  constexpr const_reference operator*() const noexcept {
    return static_cast<FwdRef<T>>(*this);
  }

  constexpr FwdListIterator& operator++() noexcept {
    node_ = node_ ? node_->next : nullptr;
    return *this;
  }

  constexpr FwdListIterator operator++(int) noexcept {
    FwdListIterator temp = *this;
    ++*this;
    return temp;
  }

  friend constexpr bool operator==(const FwdListIterator<T>&, FwdListSentinel<T>);

  friend constexpr bool operator!=(const FwdListIterator<T>&, FwdListSentinel<T>);

  constexpr operator FwdRef<const T>() const noexcept {
    return FwdRef<const T>(node_->value);
  }

  constexpr operator FwdRef<T>() noexcept {
    return FwdRef<T>(node_->value);
  }

  template<typename> friend class FwdList;
  template<typename> friend class FwdListIterator;
private:

  using Node = FwdListNode<std::remove_const_t<T>>;
  using NodePointer = std::conditional_t<std::is_const_v<T>, const Node*, Node*>;

  static constexpr FwdListIterator<const T> from_ref(const FwdRef<std::remove_const_t<T>>& ref) {
    return FwdListIterator<const T>{Node::node_from_value(ref.operator->())};
  }

  constexpr explicit FwdListIterator(NodePointer node) noexcept
    : node_(node) {}

  NodePointer node_;
};

template<typename T> constexpr bool operator==(const FwdListIterator<T>& it, FwdListSentinel<T>) {
    return it.node_ == nullptr;
}

template<typename T> constexpr bool operator!=(const FwdListIterator<T>& it, FwdListSentinel<T>) {
    return it.node_ != nullptr;
}

template<typename T> constexpr bool operator==(FwdListSentinel<T>, const FwdListIterator<T>& it) {
    return it.node_ == nullptr;
}

template<typename T> constexpr bool operator!=(FwdListSentinel<T>, const FwdListIterator<T>& it) {
    return it.node_ != nullptr;
}

template<typename T> class FwdList {
public:
  using value_type = T;
  using iterator = FwdListIterator<T>;
  using const_iterator = FwdListIterator<const T>;
  using sentinel_type = FwdListSentinel<T>;
  using Node = FwdListNode<T>;
  using ConstNode = FwdListNode<const T>;

  constexpr FwdList() noexcept = default;

  constexpr FwdList(const FwdList&) = delete;

  constexpr FwdList(FwdList&& other) noexcept
    : head_(other.head_), tail_(other.tail_), _size(0) {
    other.head_ = nullptr;
    other.tail_ = nullptr;
  }

  constexpr ~FwdList() noexcept {
    clear();
  }

  constexpr FwdList& operator=(FwdList&& other) noexcept {
    swap(other);
    return *this;
  }

  constexpr bool empty() const noexcept {
    return _size == 0;
  }

  constexpr std::size_t size() const noexcept { return _size; }

  constexpr iterator begin() noexcept {
    return iterator(head_);
  }

  constexpr const_iterator begin() const noexcept {
    return cbegin();
  }

  constexpr const_iterator cbegin() const noexcept {
    return const_iterator(head_);
  }

  constexpr sentinel_type end() noexcept {
    return {};
  }

  constexpr sentinel_type end() const noexcept {
    return {};
  }

  constexpr sentinel_type cend() const noexcept {
    return {};
  }

  constexpr T& front() noexcept {
    return head_->value;
  }

  constexpr const T& front() const noexcept {
    return head_->value;
  }

  constexpr iterator push_back(const T& value) {
    Node* node = new Node(value);
    node->next = nullptr;
    node->prev = tail_;
    if (!head_) {
      head_ = tail_ = node;
    } else {
      tail_->next = node;
      tail_ = node;
    }
    _size++;
    return iterator(node);
  }

  constexpr iterator push_back(T&& value) {
    Node* node = new Node(std::move(value));
    node->next = nullptr;
    node->prev = tail_;
    if (!head_) {
      head_ = tail_ = node;
    } else {
      tail_->next = node;
      tail_ = node;
    }
    _size++;
    return iterator(node);
  }

  template<typename... Args>
  constexpr iterator emplace_back(Args&&... args) {
    Node* node = new Node(std::forward<Args>(args)...);
    node->next = nullptr;
    node->prev = tail_;
    if (!head_) {
      head_ = tail_ = node;
    } else {
      tail_->next = node;
      tail_ = node;
    }
    _size++;
    return iterator(node);
  }

  constexpr iterator erase(const_iterator position) noexcept {
    if (position.node_ == nullptr) {
      return iterator(nullptr);
    }

    const Node* removed = position.node_;
    Node* next = removed->next;

    if (removed->prev) {
      removed->prev->next = next;
    } else {
      head_ = next;
    }

    if (next) {
      next->prev = removed->prev;
    } else {
      tail_ = removed->prev;
    }

    iterator result(next);
    delete removed;
    _size--;
    return result;
  }

  constexpr iterator erase(iterator position) noexcept {
    return erase(const_iterator(position.node_));
  }

  constexpr iterator erase(const FwdRef<T>& position) noexcept {
    return erase(const_iterator::from_ref(position));
  }

  constexpr iterator erase_front() noexcept {
    if (!head_) {
      return iterator(nullptr);
    }

    Node* removed = head_;
    head_ = head_->next;
    if (head_) {
      head_->prev = nullptr;
    } else {
      tail_ = nullptr;
    }
    iterator result(head_);
    delete removed;
    _size--;
    return result;
  }

  constexpr void clear() noexcept {
    while (head_) {
      Node* next = head_->next;
      delete head_;
      head_ = next;
    }
    tail_ = nullptr;
    _size = 0;
  }

  constexpr void swap(FwdList& other) noexcept {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(_size, other._size);
  }

private:
  Node* head_ = nullptr;
  Node* tail_ = nullptr;

  std::size_t _size;
};
