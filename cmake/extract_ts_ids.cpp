#include "CppQueries.hpp"

#include <tree_sitter/api.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cctype>

using namespace std;

extern "C" const TSLanguage* tree_sitter_cuda();

string to_upper(string_view sv) {
  string result;
  result.reserve(sv.size()); // Ottimizzazione: pre-alloca la memoria

  // Applica toupper a ogni carattere del view e lo inserisce nel risultato
  transform(sv.begin(), sv.end(), back_inserter(result),
                  [](unsigned char c) { return toupper(c); });

  return result;
}

void extract_from_query(string_view query_str) {
  uint32_t error_offset;
  TSQueryError error_type;
  TSQuery* query = ts_query_new(tree_sitter_cuda(), query_str.data(),
      uint32_t(query_str.length()), &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    string_view from(query_str.data() + error_offset,
        min(size_t(40), query_str.length() - error_offset));
    cerr << "problems with query '" << from << "'..." << endl;
    std::exit(1);
  }

  uint32_t count = ts_query_capture_count(query);

  for (uint32_t i = 0; i < count; i++) {
    uint32_t name_len;
    const char *name = ts_query_capture_name_for_id(query, i, &name_len);
    string_view name_view(name, name_len);
    cout << to_upper(name_view) << " = " << i;
    if (i < count - 1) {
      cout << ",";
    }
    cout << endl;
  }
}

int main(int argc, char** argv) {
  extract_from_query(QUERY_CPP);
  return 0;
}
