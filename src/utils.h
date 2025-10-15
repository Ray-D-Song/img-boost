#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace imgboost {

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

inline std::vector<uint8_t> base64_decode(const std::string &encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  uint8_t char_array_4[4], char_array_3[3];
  std::vector<uint8_t> ret;

  while (in_len-- && (encoded_string[in_] != '=') &&
         (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') ||
          (encoded_string[in_] == '/'))) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++)
      ret.push_back(char_array_3[j]);
  }

  return ret;
}

// URL decode
inline std::string url_decode(const std::string &str) {
  std::string result;
  for (size_t i = 0; i < str.length(); ++i) {
    if (str[i] == '%') {
      if (i + 2 < str.length()) {
        int value;
        std::sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
        result += static_cast<char>(value);
        i += 2;
      }
    } else if (str[i] == '+') {
      result += ' ';
    } else {
      result += str[i];
    }
  }
  return result;
}

// Parse with default value and upper and lower limits
inline int parse_int_param(const std::string &value, int default_value,
                           int min_value, int max_value) {
  if (value.empty()) {
    return default_value;
  }
  try {
    int val = std::stoi(value);
    if (val < min_value)
      return min_value;
    if (val > max_value)
      return max_value;
    return val;
  } catch (...) {
    return default_value;
  }
}

} // namespace imgboost
