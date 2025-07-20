// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "lexer.h"

#include <base/logging.h>
#include <base/compiler.h>
#include <base/arch.h>
// #include <base/string_utils.h>

namespace aki {

namespace {
constexpr char kLogTag[] = "lexer";

bool IsAsciiHexDigit(char8_t c) {
  if ((c >= u8'0' && c <= u8'9') || (c >= u8'a' && c <= u8'f') ||
      (c >= u8'A' && c <= u8'F')) {
    return true;
  }
  return false;
}

bool IsAsciiDigit(char8_t c) {
  return c >= u8'0' && c <= u8'9';
}

bool IsAsciiBinDigit(char8_t c) {
  return c == u8'0' || c == u8'1';
}

bool IsAsciiOctDigit(char8_t c) {
  return c >= u8'0' && c <= u8'7';
}

bool IsAsciiAlphabetic(char8_t c) {
  return (c >= u8'A' && c <= u8'Z') || (c >= u8'a' && c <= u8'z');
}

bool IsAsciiAlphaNumeric(char8_t c) {
  return IsAsciiDigit(c) || IsAsciiAlphabetic(c);
}

bool IsValidIdChar(char8_t c, bool is_first_char) {
  if (IsAsciiAlphaNumeric(c))
    return true;
  if (c == u8'_')
    return true;
  if (!is_first_char) {
    return c == u8'$' || c == u8'@' || c == u8'!' || c == u8'?';
  }
  return false;
}

}  // namespace

void LexQuotedString(TokenList& tokens,
                     const base::StringRefU8 text,
                     mem_size& index,
                     TokenType string_type,
                     mem_size opener_len) {
  auto make_ref = [text](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  const mem_size content_start = index + opener_len;
  mem_size current_pos = content_start;
  bool escaped = false;

  while (current_pos < text.length()) {
    if (text[current_pos] == u8'\\' && !escaped) {
      escaped = true;
    } else if (text[current_pos] == u8'"' && !escaped) {
      // Found the end of the string
      tokens.emplace_back(string_type, make_ref(content_start, current_pos));
      index = current_pos + 1;  // Move index past the closing quote
      return;
    } else if (text[current_pos] == u8'\r' || text[current_pos] == u8'\n') {
      BASE_LOGE(kLogTag, "Unterminated string literal (newline)");
      // Create a token for the unterminated part for better error reporting
      tokens.emplace_back(TokenType::Invalid, make_ref(content_start, current_pos));
      index = current_pos;
      return;
    } else {
      escaped = false;
    }
    current_pos++;
  }

  // If we exit the loop, the string was not terminated
  BASE_LOGE(kLogTag, "Unterminated string literal (EOF)");
  tokens.emplace_back(TokenType::Invalid, make_ref(content_start, current_pos));
  index = current_pos;
}

bool Lexer::Parse(const base::StringRefU8 text) {
  auto make_ref = [text](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

#define HANDLE_SINGLE(c, type)                                   \
  case c: {                                                            \
    tokens_.emplace_back(TokenType::type, make_ref(index, index + 1)); \
    index++;                                                           \
    break;                                                             \
  }

  mem_size index = 0;
  while (index < text.length()) {
    char8_t c = text[index];
    switch (c) {
      case u8'\r':
      case u8' ':
      case u8'\t':
        index++;
        break;
    HANDLE_SINGLE(u8'\n', Eol)
    HANDLE_SINGLE(u8';', Semicolon)
    HANDLE_SINGLE(u8',', Comma)
    HANDLE_SINGLE(u8'(', LParen)
    HANDLE_SINGLE(u8')', RParen)
    HANDLE_SINGLE(u8'{', LCurly)
    HANDLE_SINGLE(u8'}', RCurly)
    HANDLE_SINGLE(u8'[', LSquare)
    HANDLE_SINGLE(u8']', RSquare)
    HANDLE_SINGLE(u8'$', Dollar)
    HANDLE_SINGLE(u8'~', Tilde)
    HANDLE_SINGLE(u8'#', Hash)

      case u8':': {
        auto start_idx = index;
        index += 1;
        if (index < text.length() && text[index] == u8':') {
          index += 1;
          tokens_.emplace_back(TokenType::ColonColon, make_ref(start_idx, index));
        } else {
          tokens_.emplace_back(TokenType::Colon, make_ref(start_idx, start_idx + 1));
        }
        break;
      }
      case u8'"': {
        // A simple, non-prefixed string. Opener length is 1 (the quote itself).
        LexQuotedString(tokens_, text, index, TokenType::QuotedString, 1);
        break;
      }
      case u8'u': {
        // Look ahead for u16", u32", or u8" prefixes.
        // We must check for longer prefixes first to correctly parse "u16" vs "u8".
        if (index + 3 < text.length() && text[index + 1] == u8'1' &&
            text[index + 2] == u8'6' && text[index + 3] == u8'"') {
          // It's a u16 string. Opener length is 4 ("u16"").
          LexQuotedString(tokens_, text, index, TokenType::QuotedStringU16, 4);

        } else if (index + 3 < text.length() && text[index + 1] == u8'3' &&
                   text[index + 2] == u8'2' && text[index + 3] == u8'"') {
          // It's a u32 string. Opener length is 4 ("u32"").
          LexQuotedString(tokens_, text, index, TokenType::QuotedStringU32, 4);

        } else if (index + 2 < text.length() && text[index + 1] == u8'8' &&
                   text[index + 2] == u8'"') {
          // It's a u8 string, which we treat as a regular QuotedString.
          // The "opener" is 3 characters long (u8").
          LexQuotedString(tokens_, text, index, TokenType::QuotedString, 3);

        } else {
          // If it's not a recognized string prefix, it must be an identifier.
          // This correctly handles variables like "user_id" or a standalone "u8".
          LexIdentifier(tokens_, text, index);
        }
        break;
      }
      case u8'+': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            tokens_.emplace_back(TokenType::PlusEquals, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'+') {
            tokens_.emplace_back(TokenType::PlusPlus, make_ref(start, ++index));
            continue;
          }
        }
        tokens_.emplace_back(TokenType::Plus, make_ref(start, start + 1));
        break;
      }
      case u8'-': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            tokens_.emplace_back(TokenType::MinusEquals, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'-') {
            tokens_.emplace_back(TokenType::MinusMinus, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'>') {
            tokens_.emplace_back(TokenType::Arrow, make_ref(start, ++index));
            continue;
          }
        }
        tokens_.emplace_back(TokenType::Minus, make_ref(start, start + 1));
        break;
      }
      case u8'*': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'=') {
          tokens_.emplace_back(TokenType::AsteriskEqual, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::Asterisk, make_ref(start, start + 1));
        break;
      }
      case u8'/': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            tokens_.emplace_back(TokenType::ForwardSlashEqual, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'/') {
            // Line comment
            auto comment_start = index - 1;
            while (index < text.length()) {
              if (text[index] == u8'\n') {
                // Don't include newline
                tokens_.emplace_back(TokenType::LineComment,
                                     make_ref(comment_start, index));
                break;
              }
              index++;
            }
            if (index == text.length()) {
              tokens_.emplace_back(TokenType::LineComment,
                                   make_ref(comment_start, index));
            }
            index++;  // Skip newline or EOF
            continue;
          } else if (text[index] == u8'*') {
            // Block comment
            auto comment_start = index - 1;
            index++;                 // Skip initial asterisk
            bool found_end = false;  // <-- ADD a flag
            while (index < text.length()) {
              if (text[index] == u8'*' && index + 1 < text.length() &&
                  text[index + 1] == u8'/') {
                tokens_.emplace_back(TokenType::BlockComment,
                                     make_ref(comment_start, index + 2));
                index += 2;        // Skip closing */
                found_end = true;  // <-- SET the flag
                break;
              }
              index++;
            }
            // Only create an unclosed comment token if the end was NOT found
            if (!found_end) {  // <-- CHECK the flag
              // This implies the while loop finished because index >= text.length()
              tokens_.emplace_back(TokenType::BlockComment,
                                   make_ref(comment_start, index));
            }
            continue;
          }
        }
        tokens_.emplace_back(TokenType::ForwardSlash, make_ref(start, start + 1));
        break;
      }
      case u8'=': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            tokens_.emplace_back(TokenType::DoubleEqual, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'>') {
            tokens_.emplace_back(TokenType::FatArrow, make_ref(start, ++index));
            continue;
          }
        }
        tokens_.emplace_back(TokenType::Equal, make_ref(start, start + 1));
        break;
      }
      case u8'>': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            tokens_.emplace_back(TokenType::GreaterThanOrEqual, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'>') {
            index++;
            if (index < text.length() && text[index] == u8'=') {
              tokens_.emplace_back(TokenType::RightShiftEqual, make_ref(start, ++index));
              continue;
            } else if (index < text.length() && text[index] == u8'>') {
              tokens_.emplace_back(TokenType::RightArithmeticShift,
                                   make_ref(start, ++index));
              continue;
            } else {
              tokens_.emplace_back(TokenType::RightShift, make_ref(start, index));
              continue;
            }
          }
        }
        tokens_.emplace_back(TokenType::GreaterThan, make_ref(start, start + 1));
        break;
      }
      case u8'<': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            tokens_.emplace_back(TokenType::LessThanOrEqual, make_ref(start, ++index));
            continue;
          } else if (text[index] == u8'<') {
            index++;
            if (index < text.length() && text[index] == u8'=') {
              tokens_.emplace_back(TokenType::LeftShiftEqual, make_ref(start, ++index));
              continue;
            } else if (index < text.length() && text[index] == u8'<') {
              tokens_.emplace_back(TokenType::LeftArithmeticShift,
                                   make_ref(start, ++index));
              continue;
            } else {
              tokens_.emplace_back(TokenType::LeftShift, make_ref(start, index));
              continue;
            }
          }
        }
        tokens_.emplace_back(TokenType::LessThan, make_ref(start, start + 1));
        break;
      }
      case u8'!': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'=') {
          tokens_.emplace_back(TokenType::NotEqual, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::Not, make_ref(start, start + 1));
        break;
      }
      case u8'&': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'&') {
          tokens_.emplace_back(TokenType::LogicalAnd, make_ref(start, ++index));
          continue;
        } else if (index < text.length() && text[index] == u8'=') {
          tokens_.emplace_back(TokenType::AmpersandEqual, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::Ampersand, make_ref(start, start + 1));
        break;
      }
      case u8'|': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'|') {
          tokens_.emplace_back(TokenType::LogicalOr, make_ref(start, ++index));
          continue;
        } else if (index < text.length() && text[index] == u8'=') {
          tokens_.emplace_back(TokenType::PipeEqual, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::Pipe, make_ref(start, start + 1));
        break;
      }
      case u8'^': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'=') {
          tokens_.emplace_back(TokenType::CaretEqual, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::Caret, make_ref(start, start + 1));
        break;
      }
      case u8'%': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'=') {
          tokens_.emplace_back(TokenType::PercentSignEqual, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::PercentSign, make_ref(start, start + 1));
        break;
      }
      case u8'?': {
        auto start = index;
        index++;
        if (index < text.length() && text[index] == u8'?') {
          index++;
          if (index < text.length() && text[index] == u8'=') {
            tokens_.emplace_back(TokenType::NullishCoalescingEqual,
                                 make_ref(start, ++index));
            continue;
          }
          tokens_.emplace_back(TokenType::NullishCoalescing, make_ref(start, index));
          continue;
        } else if (index < text.length() && text[index] == u8'.') {
          tokens_.emplace_back(TokenType::QuestionDot, make_ref(start, ++index));
          continue;
        }
        tokens_.emplace_back(TokenType::QuestionMark, make_ref(start, start + 1));
        break;
      }
      case u8'.': {
        auto start = index;
        index++;
        if (index < text.length()) {
          if (text[index] == u8'.') {
            index++;
            if (index < text.length() && text[index] == u8'.') {
              tokens_.emplace_back(TokenType::Ellipsis, make_ref(start, ++index));
              continue;
            }
            tokens_.emplace_back(TokenType::Range, make_ref(start, index));
            continue;
          } else if (IsAsciiDigit(text[index])) {
            // Leading dot followed by digits
            LexNumber(tokens_, text, index, start, true);  // don't return!
            continue;         
          }
        }
        tokens_.emplace_back(TokenType::Dot, make_ref(start, start + 1));
        break;
      }
      case u8'`': {
        auto start = index;
        bool escaped = false;
        int braces = 0;

        index++;  // Skip initial backtick
        while (index < text.length()) {
          if (text[index] == u8'\\' && !escaped) {
            escaped = true;
            index++;
            continue;
          }

          if (escaped) {
            escaped = false;
            index++;
            continue;
          }

          if (text[index] == u8'`') {
            break;
          } else if (text[index] == u8'$' && index + 1 < text.length() &&
                     text[index + 1] == u8'{') {
            braces++;
            index += 2;
          } else if (text[index] == u8'}' && braces > 0) {
            braces--;
            index++;
          } else {
            index++;
          }
        }

        if (index >= text.length() || text[index] != u8'`') {
          BASE_LOGE(kLogTag, "Unterminated template literal");
          return false;
        }

        tokens_.emplace_back(TokenType::TemplateLiteral, make_ref(start, index + 1));
        index++;  // Skip closing backtick
        break;
      }
      default: {
        if (IsAsciiDigit(c)) {
          LexNumber(tokens_, text, index, index, false);
        } else if (c == u8'_' || IsAsciiAlphabetic(c)) {
          LexIdentifier(tokens_, text, index);
        } else {
          BASE_LOGE(kLogTag, "Unrecognized character: {}", static_cast<char>(c));
          tokens_.emplace_back(TokenType::Invalid, make_ref(index, index + 1));
          index++;
        }
        break;
      }
    }  // end switch
  }    // end while

  tokens_.emplace_back(TokenType::Eof, make_ref(index, index));
  return true;
}

void LexIdentifier(TokenList& tokens, const base::StringRefU8 text, mem_size& index) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  auto start = index;
  bool is_first_char = true;
  bool escaped = false;

  while (index < text.length()) {
    if (escaped) {
      escaped = false;
      // After backslash, accept any character except non-printable
      if (text[index] >= 0x20 && text[index] <= 0x7E) {
        index++;
      } else {
        break;
      }
      continue;
    }

    if (text[index] == u8'\\') {
      escaped = true;
      index++;
      continue;
    }

    if (IsValidIdChar(text[index], is_first_char)) {
      is_first_char = false;
      index++;
    } else {
      break;
    }

    is_first_char = false;
  }

  auto id_ref = make_ref(start, index);

  // Check for keywords
  TokenType type = TokenType::Identifier;
  if (id_ref == u8"fn")
    type = TokenType::FnKeyword;
  else if (id_ref == u8"let")
    type = TokenType::LetKeyword;
  else if (id_ref == u8"const")
    type = TokenType::ConstKeyword;
  else if (id_ref == u8"if")
    type = TokenType::IfKeyword;
  else if (id_ref == u8"else")
    type = TokenType::ElseKeyword;
  else if (id_ref == u8"for")
    type = TokenType::ForKeyword;
  else if (id_ref == u8"while")
    type = TokenType::WhileKeyword;
  else if (id_ref == u8"return")
    type = TokenType::ReturnKeyword;
  else if (id_ref == u8"true" || id_ref == u8"false")
    type = TokenType::BoolLiteral;

  tokens.emplace_back(Token(type, id_ref));
}


inline bool IsOctalDigit(char8_t c) {
  return c >= u8'0' && c <= u8'7';
}
inline bool IsBinaryDigit(char8_t c) {
  return c == u8'0' || c == u8'1';
}
inline bool IsHexDigit(char8_t c) {
  return IsAsciiDigit(c) || (c >= u8'a' && c <= u8'f') || (c >= u8'A' && c <= u8'F');
}

// non static for unit testing purposes
bool LexNumber(TokenList& tokens,
               const base::StringRefU8 text,
               mem_size& index,
               mem_size custom_start,
               bool has_leading_dot) {
  auto make_ref = [text](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  // Helper to check for a suffix without going out of bounds
  auto check_suffix = [text](mem_size pos, const char* suffix) {
    base::StringRefU8 s_ref(reinterpret_cast<const char8_t*>(suffix));
    if (pos + s_ref.length() > text.length())
      return false;
    return memcmp(&text.data()[pos], s_ref.data(), s_ref.length()) == 0;
  };

  const mem_size start = custom_start != index ? custom_start : index;
  bool is_floating_point = has_leading_dot;
  bool has_digit = false;
  bool has_exponent = false;
  mem_size number_body_start = start;  // Will be moved past the prefix if one exists

  // Handle Hex, Binary, and Octal Prefixes
  if (!has_leading_dot && index == start && text[index] == u8'0' &&
      index + 1 < text.length()) {
    char8_t prefix = text[index + 1];
    bool (*is_valid_digit)(char8_t) = nullptr;

    if (prefix == u8'x' || prefix == u8'X')
      is_valid_digit = &IsHexDigit;
    else if (prefix == u8'b' || prefix == u8'B')
      is_valid_digit = &IsBinaryDigit;
    else if (prefix == u8'o' || prefix == u8'O')
      is_valid_digit = &IsOctalDigit;

    if (is_valid_digit) {
      index += 2;  // Consume prefix
      number_body_start = index;

      while (index < text.length() &&
             (is_valid_digit(text[index]) || text[index] == u8'_')) {
        if (text[index] != u8'_')
          has_digit = true;
        index++;
      }

      if (!has_digit) { /* Error handling for "0x" with no digits */
      }
      // This is an integer, so we can skip float/exponent parsing
      tokens.emplace_back(TokenType::IntegerNumber, make_ref(number_body_start, index));
      return true;
    }
  }

  // Integer part
  mem_size integer_end = index;
  while (integer_end < text.length() &&
         (IsAsciiDigit(text[integer_end]) || text[integer_end] == u8'_')) {
    if (text[integer_end] != u8'_')
      has_digit = true;
    integer_end++;
  }
  index = integer_end;

  // Fractional part
  if (index < text.length() && text[index] == u8'.') {
    if (index + 1 < text.length() && IsAsciiDigit(text[index + 1])) {
      is_floating_point = true;
      index++;  // consume '.'
      while (index < text.length() &&
             (IsAsciiDigit(text[index]) || text[index] == u8'_')) {
        has_digit = true;
        index++;
      }
    }
  }

  // Exponent part
  if (index < text.length() && (text[index] == u8'e' || text[index] == u8'E')) {
    is_floating_point = true;
    has_exponent = true;
    index++;

    // Exponent sign
    if (index < text.length() && (text[index] == u8'+' || text[index] == u8'-')) {
      index++;
    }

    // Must have at least one digit after exponent marker
    if (index < text.length() && IsAsciiDigit(text[index])) {
      has_digit = true;
      index++;
    } else {
      BASE_LOGE(kLogTag, "Missing exponent value");
      return false;
    }

    while (index < text.length()) {
      if (IsAsciiDigit(text[index])) {
        index++;
      } else if (text[index] == u8'_' && index + 1 < text.length() &&
                 IsAsciiDigit(text[index + 1])) {
        index += 2;
      } else {
        break;
      }
    }
  }


  if (!has_digit) {
    return false;
  }

  // try to determine the type based on the suffix
  if (index < text.length()) {
    char8_t suffix_char = text[index];
    if (suffix_char == u8'u' || suffix_char == u8'i' || suffix_char == u8'f') {
      // Check for multi-character suffixes first
      if (check_suffix(index, "u64") || check_suffix(index, "u32") ||
          check_suffix(index, "u16") || check_suffix(index, "u8")) {
        index += 3;  // uXX
      } else if (check_suffix(index, "i64") || check_suffix(index, "i32") ||
                 check_suffix(index, "i16") || check_suffix(index, "i8")) {
        index += 3;  // iXX
      } else if (check_suffix(index, "f64") || check_suffix(index, "f32")) {
        is_floating_point = true;
        index += 3;  // fXX
      } else {
        // Fallback to single-character suffix
        index++;
        if (suffix_char == u8'f') {
          is_floating_point = true;
        }
      }
    }
  }

  TokenType final_type =
      is_floating_point ? TokenType::FloatNumber : TokenType::IntegerNumber;
  // NOTE: tokens contain the suffix within their value
  tokens.emplace_back(Token(final_type, make_ref(start, index)));
  return true;
}

}  // namespace aki
