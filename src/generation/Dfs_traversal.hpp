#pragma once

#include <coroutine>
#include <optional>
#include <utility>
#include <stack>

template<typename RetType> struct CoroIterGenerator {
    struct promise_type {
        std::optional<RetType> current_value;

        CoroIterGenerator get_return_object() {
            return CoroIterGenerator{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        std::suspend_always yield_value(const RetType& value) noexcept {
            current_value.emplace(value);
            return {};
        }
        std::suspend_always yield_value(RetType&& value) noexcept {
            current_value.emplace(std::move(value));
            return {};
        }
        void return_void() {}
    };

    using handle_type = std::coroutine_handle<promise_type>;
    handle_type h;

    CoroIterGenerator(handle_type h) : h(h) {}
    ~CoroIterGenerator() { if (h) h.destroy(); }

    // Iterator interface to support range-based for-loops over the generator
    struct iterator {
        handle_type h;
        bool done;
        void operator++() { h.resume(); done = h.done(); }
        RetType& operator*() { return h.promise().current_value.value(); }
        bool operator!=(const iterator& other) const { return done != other.done; }
    };

    iterator begin() { h.resume(); return iterator{h, h.done()}; }
    iterator end() { return iterator{h, true}; }
};

template<typename T> concept dfs_stack_controller = requires (T& c) {
  typename T::yield_value;
  typename T::stack_value;
  c.make_init();
  requires std::same_as<typename T::stack_value, std::decay_t<decltype(c.make_init())>>;

  c.make_yield(std::declval<const typename T::stack_value&>());
  requires std::same_as<typename T::yield_value, std::decay_t<decltype(c.make_yield(std::declval<const typename T::stack_value&>()))>>;

  c.make_next(std::declval<typename T::stack_value&>());
  requires std::same_as<typename T::stack_value, std::decay_t<decltype(c.make_next(std::declval<typename T::stack_value&>()))>>;

  { c.has_next(std::declval<const typename T::stack_value&>()) } -> std::convertible_to<bool>;

  c.after_pop(std::declval<typename T::stack_value&>());
};

// stack_controller_t handles potential global state to be modified during stack manipulation;
// if there is no global state, it can just re-wire the calls to the object that is on the stack, like
//
//
// bool has_next(const stack_t& top) { return top.hash_next(); }
//
// stack_t make_next(stack_t& top) { return top.make_next(); }
//
// void after_pop(stack_t&) {} // do nothing
//
// and so on.
//
// Instead, make_yield() transforms the stack top into something else, in order to co_yield to the user
// a "view" of state of the stack (and, if needed, of the controller), e.g., to expose only certain info.
// If this is not needed, simply re-wire like:
//
// const stack_t& make_yield(const stack_t& top) { return top; }
//
template<dfs_stack_controller stack_controller_t>
CoroIterGenerator<typename stack_controller_t::yield_value> traverse_dfs_preorder(stack_controller_t& controller) {
  using stack_t = typename stack_controller_t::stack_value;
  std::stack<stack_t> stack;

  stack.push(controller.make_init());
  // pre-order traversal: push and visit immediately
  co_yield controller.make_yield(stack.top());

  while (not stack.empty()) {
    stack_t& current_frame = stack.top();
    // query if the current node has unvisited children left
    if (controller.has_next(current_frame)) {
      // push the new child to the top of the stack to process it on the next loop iteration (DFS)
      stack.push(controller.make_next(current_frame));
      // pre-order traversal: push and visit immediately
      co_yield controller.make_yield(stack.top());
    } else {
      // node processing finished, safely clear it out of the stack
      stack.pop();
      if (not stack.empty()) {
        controller.after_pop(stack.top());
      }
    }
  }
}
