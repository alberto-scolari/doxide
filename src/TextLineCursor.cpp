#include <TextLineCursor.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>

TextLineCursor::TextLineCursor(const std::string_view &v): TextLineCursor(v, 0) {}

void TextLineCursor::advance(std::size_t length) {
  check_length(length);
  _line_num += _do_count_lines_in_range(cbegin(), cbegin() + length);
  _data = _data.substr(length);
}

void TextLineCursor::clear() noexcept {
  _data = std::string_view();
  _line_num = 0;
}


void TextLineCursor::check_start(const_iterator start) const {
  if (start < cbegin()) {
    throw std::out_of_range("start is before beginning");
  }
  if (start > cend()) {
    throw std::out_of_range("start is beyond size");
  }
}

void TextLineCursor::check_length(std::size_t length) const {
  if (length > size()) {
    throw std::out_of_range("length is beyond size");
  }
}

void TextLineCursor::check_end(const_iterator end) const {
  if (end > cend()) {
    throw std::out_of_range("end is beyond text");
  }
}

void TextLineCursor::check_range(std::size_t start, std::size_t length) const {
  check_start(cbegin() + start);
  check_length(start + length);
}

void TextLineCursor::check_range(const_iterator start, std::size_t length) const {
  check_start(start);
  check_end(start + length);
}

void TextLineCursor::check_range(const_iterator start, const_iterator end) const {
  if (end < start) {
    throw std::out_of_range("end is before start");
  }
  check_start(start);
  check_end(end);
}

std::size_t TextLineCursor::count_lines_in_range(const_iterator start, const_iterator end) const {
  check_range(start, end);
  return _do_count_lines_in_range(start, end);
}

std::size_t TextLineCursor::count_lines_in_range(const_iterator start, std::size_t length) const {
  check_range(start, length);
  return count_lines_in_range(start, start + length);
}

std::size_t TextLineCursor::count_lines_in_range(std::size_t start, std::size_t length) const {
  return count_lines_in_range(_data.cbegin() + start, _data.cbegin() + start + length);
}

std::size_t TextLineCursor::_do_count_lines_in_range(const_iterator start, const_iterator end) const {
  return std::count(start, end, '\n');
}

TextLineCursor TextLineCursor::substr(std::size_t start, std::size_t size) const {
  check_range(start, size);
  auto sub = _data.substr(start, size);
  auto res = TextLineCursor(sub, _line_num + count_lines_in_range(cbegin(), start));
  return res;
}

TextLineCursor TextLineCursor::substr(std::size_t start) const {
  std::size_t len = size() - start;
  check_range(start, len);
  auto sub = _data.substr(start, len);
  auto res = TextLineCursor(sub, _line_num + count_lines_in_range(cbegin(), start));
  return res;
}

TextLineCursor TextLineCursor::substr(const_iterator start, std::size_t length) const {
  check_range(start, length);
  auto sub = _data.substr(std::distance(cbegin(), start), length);
  return TextLineCursor(sub, _line_num + count_lines_in_range(cbegin(), start));
}
