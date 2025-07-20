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
      case u8'"':
      case u8'u':
      case u8'U': {
        // Check for string prefixes
        if (c == u8'u' || c == u8'U') {
          if (index + 1 >= text.length() || text[index + 1] != u8'"') {
            // Not a string prefix, handle as identifier
            LexIdentifier(tokens_, text, index);
            break;
          }
          // Fall through to handle as string
        }

        TokenType string_type = TokenType::QuotedString;
        mem_size prefix_len = (c == u8'"') ? 1 : 0;

        if (c == u8'u') {
          string_type = TokenType::QuotedStringU16;
          prefix_len = 2;
        } else if (c == u8'U') {
          string_type = TokenType::QuotedStringU32;
          prefix_len = 2;
        }

        auto start = index;
        index += prefix_len;  // Skip prefix

        bool escaped = false;
        u32 unicode_escape = 0;
        mem_size escape_index = 0;

        while (index < text.length()) {
          if (text[index] == u8'\\' && !escaped) {
            escaped = true;
            escape_index = index;
            unicode_escape = 0;
            index++;
            continue;
          } else if (escaped) {
            if (text[index] == u8'u') {
              // Start of Unicode escape sequence
              escape_index = index;
              unicode_escape = 1;
              index++;
            } else if (unicode_escape > 0 && unicode_escape <= 4) {
              // Continue Unicode escape sequence
              if (!IsAsciiHexDigit(text[index])) {
                BASE_LOGE(kLogTag, "Invalid hex digit in Unicode escape");
                return false;
              }
              unicode_escape++;
              if (unicode_escape > 4) {
                escaped = false;
              }
              index++;
            } else {
              // Other escape sequence
              escaped = false;
              index++;
            }
          } else if (text[index] == u8'"') {
            break;
          } else if (text[index] == u8'\r' || text[index] == u8'\n') {
            BASE_LOGE(kLogTag, "Unterminated string literal (newline)");
            return false;
          } else {
            index++;
          }
        }

        if (index >= text.length() || text[index] != u8'"') {
          BASE_LOGE(kLogTag, "Unterminated string literal");
          return false;
        }

        tokens_.emplace_back(Token(string_type, make_ref(start + prefix_len, index)));
        index++;  // Skip closing quote
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
            index++;  // Skip initial asterisk
            while (index < text.length()) {
              if (text[index] == u8'*' && index + 1 < text.length() &&
                  text[index + 1] == u8'/') {
                tokens_.emplace_back(TokenType::BlockComment,
                                     make_ref(comment_start, index + 2));
                index += 2;  // Skip closing */
                break;
              }
              index++;
            }
            if (index >= text.length()) {
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
            return LexNumber(tokens_, text, index, start, true);
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

// non static for unit testing purposes
bool LexNumber(TokenList& tokens,
               const base::StringRefU8 text,
               mem_size& index,
               mem_size custom_start,
               bool has_leading_dot) {
  auto make_ref = [text](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  const mem_size start = custom_start != index ? custom_start : index;
  bool is_floating_point = has_leading_dot;
  bool has_exponent = false;
  bool has_digit = false;  // Ensures number has at least one digit

  // Handle leading sign (for exponent only)
  if (custom_start == index && index < text.length() &&
      (text[index] == u8'+' || text[index] == u8'-')) {
    index++;
  }

  // Integer part
  if (!has_leading_dot) {
    while (index < text.length()) {
      if (IsAsciiDigit(text[index])) {
        has_digit = true;
        index++;
      } else if (text[index] == u8'_' && index + 1 < text.length() &&
                 IsAsciiDigit(text[index + 1])) {
        // Valid underscore (between digits)
        index += 2;
        has_digit = true;
      } else {
        break;
      }
    }
  }

  // Fractional part
  if (index < text.length() && text[index] == u8'.') {
    is_floating_point = true;
    index++;
    while (index < text.length()) {
      if (IsAsciiDigit(text[index])) {
        index++;
        has_digit = true;
      } else if (text[index] == u8'_' && index + 1 < text.length() &&
                 IsAsciiDigit(text[index + 1])) {
        index += 2;
        has_digit = true;
      } else {
        break;
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
    BASE_LOGE(kLogTag, "Invalid numeric literal");
    tokens.emplace_back(TokenType::InvalidNumber, make_ref(start, index));
    return true;
  }

  // Numeric suffix (optional)
  if (index < text.length()) {
    char8_t suffix = text[index];
    if (suffix == u8'u' || suffix == u8'i' || suffix == u8'f') {
      TokenType suffix_type = TokenType::IntegerNumber;
      if (suffix == u8'f') {
        suffix_type = TokenType::FloatNumber;
        is_floating_point = true;
      }
      index++;
      tokens.emplace_back(Token(suffix_type, make_ref(start, index)));
      return true;
    }
  }

  TokenType base_type =
      is_floating_point ? TokenType::FloatNumber : TokenType::IntegerNumber;
  tokens.emplace_back(Token(base_type, make_ref(start, index)));
  return true;
}

}  // namespace aki
