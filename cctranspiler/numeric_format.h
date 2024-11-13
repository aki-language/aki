#pragma once
#include <grammar/token.h>

namespace insane {
// Helper function to convert an integer to a string
inline base::StringU8 IntToString(int value) {
  base::StringU8 result;
  bool negative = value < 0;
  if (negative) {
    value = -value;
  }
  do {
    char8_t digit = static_cast<char8_t>(value % 10 + '0');
    result = base::StringU8(&digit, 1) + result;
    value /= 10;
  } while (value > 0);
  if (negative) {
    result = base::StringU8(u8"-") + result;
  }
  return result;
}

inline base::StringU8 ConvertNumericRepresentation(
    const insane::TokenType tt,
    const base::StringRefU8 data) {
  if (tt == insane::TokenType::BinaryNumber) {
    int decimal = 0;
    for (size_t i = 0; i < data.size(); i++) {
      if (data[i] == '1') {
        decimal = decimal * 2 + 1;
      } else if (data[i] == '0') {
        decimal = decimal * 2;
      } else {
        // Error: Invalid binary character
        return u8"";
      }
    }
    return base::StringU8(IntToString(decimal));
  } else if (tt == insane::TokenType::HexNumber) {
    int decimal = 0;
    for (size_t i = 0; i < data.size(); i++) {
      int digit = 0;
      if (data[i] >= '0' && data[i] <= '9') {
        digit = data[i] - '0';
      } else if (data[i] >= 'A' && data[i] <= 'F') {
        digit = data[i] - 'A' + 10;
      } else if (data[i] >= 'a' && data[i] <= 'f') {
        digit = data[i] - 'a' + 10;
      } else {
        // Error: Invalid hexadecimal character
        return u8"";
      }
      decimal = decimal * 16 + digit;
    }
    return base::StringU8(IntToString(decimal));
  } else if (tt == insane::TokenType::OctalNumber) {
    int decimal = 0;
    for (size_t i = 0; i < data.size(); i++) {
      if (data[i] >= '0' && data[i] <= '7') {
        decimal = decimal * 8 + (data[i] - '0');
      } else {
        // Error: Invalid octal character
        return u8"";
      }
    }
    return base::StringU8(IntToString(decimal));
  }
  // Error: Unknown token type
  return u8"";
}
}