#pragma once

#include <functional>

template<typename T>
class FwdListIterator;

class FwdHashable {
public:
  FwdHashable() = delete;

  constexpr FwdHashable(const FwdHashable&) = default;

  constexpr bool operator==(const FwdHashable& other) const noexcept {
    return ptr == other.ptr;
  }

  std::size_t hash() const noexcept {
    return reinterpret_cast<std::size_t>(ptr);
  }

  template<typename T> friend class FwdRef;

private:
  constexpr FwdHashable(void* p): ptr(p) {}

  void* ptr;
};

namespace std {
    template<> struct hash<FwdHashable> {
        std::size_t operator()(const FwdHashable& p) const noexcept {
            return p.hash();
        }
    };
}

template<typename T>
class FwdRef {
public:
  using value_type = T;

  FwdRef() = delete;

  constexpr FwdRef(const FwdRef<T>& other) noexcept = default;

  constexpr FwdRef& operator=(const FwdRef<T>& other) noexcept = default;

  constexpr T& operator*() noexcept {
    return *ptr_;
  }

  constexpr const T* operator->() const noexcept {
    return ptr_;
  }

  constexpr T* operator->() noexcept {
    return ptr_;
  }

  constexpr explicit operator bool() const noexcept {
    return ptr_ != nullptr;
  }

  operator FwdHashable() const noexcept {
    return FwdHashable(ptr_);
  }

  template<typename> friend class FwdListIterator;
private:

  constexpr FwdRef(T& p) noexcept: ptr_(&p) {}

  T* ptr_;
};
