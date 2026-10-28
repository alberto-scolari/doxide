#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

/**
 * Write a whole file, prompting to confirm overwrite if it already exists.
 *
 * @ingroup developer
 */
void write_file_prompt(const std::string& contents, const std::filesystem::path& dst);

/**
 * Read in a whole file.
 *
 * @ingroup developer
 */
std::string gulp(const std::filesystem::path& src);

/**
 * Convert a string to lowercase with uppercase first character.
 *
 * @ingroup developer
 */
std::string capitalize(const std::string_view s);

bool can_write(const std::filesystem::path& path);

struct UpperStr {
    std::string_view view;
};

template <> struct std::formatter<UpperStr> : std::formatter<std::string_view> {
    auto format(UpperStr wrapper, format_context& ctx) const {
        // Convert to uppercase on the fly into a temporary buffer
        std::string upper;
        upper.reserve(wrapper.view.size());
        for (char c : wrapper.view) {
            upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
        // Forward the temporary string to the base string_view formatter
        return std::formatter<std::string_view>::format(upper, ctx);
    }
};

struct RepeatChar {
    int count;
    char ch = ' ';
};

template <> struct std::formatter<RepeatChar> {

    // Parses format specifiers if needed (e.g., if you wrote {:x}).
    // We don't need specifiers here, so we just return the iterator.
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    // This writes the output to the formatting buffer
    auto format(const RepeatChar& rc, std::format_context& ctx) const {
        // Efficiently write the character 'rc.count' times directly into the output
        return std::fill_n(ctx.out(), rc.count, rc.ch);
    }
};

struct het_string_hash {
    using is_transparent = void; // Enables heterogeneous lookup

    constexpr std::size_t operator()(const std::string_view& sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }

    constexpr std::size_t operator()(const std::string& s) const noexcept {
        return std::hash<std::string>{}(s);
    }
};
