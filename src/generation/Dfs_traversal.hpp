#pragma once

#include <entities/RefVariant.hpp>

#include <coroutine>
#include <filesystem>
#include <optional>
#include <string>
#include <tuple>

class EntityRegistry;

using DfsPreorderResultT = std::tuple<const RefVariant, const std::string&, const std::filesystem::path&>;

struct RefVariantGenerator {
    struct promise_type {
        std::optional<DfsPreorderResultT> current_value;

        RefVariantGenerator get_return_object() {
            return RefVariantGenerator{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        std::suspend_always yield_value(const DfsPreorderResultT& value) noexcept {
            current_value.emplace(value);
            return {};
        }
        void return_void() {}
    };

    using handle_type = std::coroutine_handle<promise_type>;
    handle_type h;

    RefVariantGenerator(handle_type h) : h(h) {}
    ~RefVariantGenerator() { if (h) h.destroy(); }

    // Iterator interface to support range-based for-loops over the generator
    struct iterator {
        handle_type h;
        bool done;
        void operator++() { h.resume(); done = h.done(); }
        DfsPreorderResultT operator*() { return h.promise().current_value.value(); }
        bool operator!=(const iterator& other) const { return done != other.done; }
    };

    iterator begin() { h.resume(); return iterator{h, h.done()}; }
    iterator end() { return iterator{h, true}; }
};

RefVariantGenerator traverse_dfs_preorder(const EntityRegistry& registry, const std::filesystem::path& output_path);
