// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "lexer.h"
#include <base/logging.h>
#include "base/arch.h"
#include "base/compiler.h"

namespace insane {

namespace {

constexpr char kTag[] = "lexer";

bool IsAsciiHexDigit(const char8_t c) {
  if ((c >= u8'0' && c <= u8'9') || (c >= u8'a' && c <= u8'f') ||
      (c >= u8'A' && c <= u8'F')) {
    return true;
  }
  return false;
}

bool IsAsciiDigit(const char8_t c) {
  return c >= u8'0' && c <= u8'9';
}

bool IsAsciiAlphabetic(const char8_t c) {
  // Check if the character is in the range 'A' to 'Z' or 'a' to 'z'
  return (c >= u8'A' && c <= u8'Z') || (c >= u8'a' && c <= u8'z');
}

bool IsAsciiAlphaNumeric(const char8_t c) {
  // Check if the character is in the range '0' to '9', 'A' to 'Z', or 'a' to 'z'
  return (c >= u8'0' && c <= u8'9') || (c >= u8'A' && c <= u8'Z') ||
         (c >= u8'a' && c <= u8'z');
}

// this should work without NTERM
u64 BinToNumber(const base::StringRefU8 number_slice_text) {
  u64 result = 0;
  for (size_t i = 0; i < number_slice_text.length(); i++) {
    if (number_slice_text[i] == u8'\n')
      return 0;

    if (number_slice_text[i] != '0' && number_slice_text[i] != '1') {
      // invalid character
      return 0;
    }
    result = (result << 1) | (number_slice_text[i] - '0');
  }

  return result;
}

LiteralSuffix ConsumeNumericLiteralSuffix(const base::StringRefU8 text,
                                          mem_size& index) {
  switch (text[index]) {
    case u8'u':  // unsigned type
    case u8'i':  // integer type
    case u8'f':  // float type
      break;
    default:
      return LiteralSuffix::NONE;
  }

  auto local_index = index + 1;
  if (local_index >= text.size()) {
    return LiteralSuffix::NONE;
  }

  if (text[index] == u8'u' && text[local_index] == u8'z') {
    index += 2;
    return LiteralSuffix::UZ;
  }

  size_t start = local_index;
  while (local_index < text.size() && IsAsciiDigit(text[local_index])) {
    local_index += 1;
  }

  return LiteralSuffix::NONE;
}
}  // namespace

bool Lexer::Parse(const base::StringRefU8 text) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  mem_size index = 0;
  while (index < text.length()) {
    char8_t c = text[index];
    // LOG_INFO("Parsing character {} at idx {}", (char)c, index);
    switch (c) {
      case u8'\r':
      case u8' ':
      case u8'\t':
        index++;
        break;
      case u8'\n':
        tokens_.emplace_back(TokenType::Eol, make_ref(index, index + 1));
        index++;
        break;
      case u8';':
        tokens_.emplace_back(TokenType::Semicolon, make_ref(index, index + 1));
        index++;
        break;
      case u8',':
        tokens_.emplace_back(TokenType::Comma, make_ref(index, index + 1));
        index++;
        break;
      case u8'(':
        tokens_.emplace_back(TokenType::LParen, make_ref(index, index + 1));
        index++;
        break;
      case u8')':
        tokens_.emplace_back(TokenType::RParen, make_ref(index, index + 1));
        index++;
        break;
      case u8'{':
        tokens_.emplace_back(TokenType::LCurly, make_ref(index, index + 1));
        index++;
        break;
      case u8'}':
        tokens_.emplace_back(TokenType::RCurly, make_ref(index, index + 1));
        index++;
        break;
      case u8'[':
        tokens_.emplace_back(TokenType::LSquare, make_ref(index, index + 1));
        index++;
        break;
      case u8']':
        tokens_.emplace_back(TokenType::RSquare, make_ref(index, index + 1));
        index++;
        break;
      case u8'$':
        tokens_.emplace_back(TokenType::Dollar, make_ref(index, index + 1));
        index++;
        break;
      case u8'~':
        tokens_.emplace_back(TokenType::Tilde, make_ref(index, index + 1));
        index++;
        break;
      case u8'#':
        tokens_.emplace_back(TokenType::Hash, make_ref(index, index + 1));
        index++;
        break;
      case u8':': {
        auto start = index;
        index += 1;
        if (index < text.length() && text[index] == u8':') {
          index += 1;
          tokens_.emplace_back(TokenType::ColonColon, make_ref(start, start + 2));
        } else {
          tokens_.emplace_back(TokenType::Colon, make_ref(start, start + 1));
        }
        break;
      }
      // quoted string
      case u8'"': {
        auto start = index;
        index += 1;

        bool escaped = false;

        while (index < text.length() && (escaped || text[index] != u8'"')) {
          if (text[index] == u8'\r' || text[index] == u8'\n') {
            index++;
            break;
          }
          index++;
        }

        if (index == text.length() || text[index] != u8'"') {
          BASE_LOGE(kTag, "Unterminated string literal");
          return false;
        }

        auto ref = make_ref(start + 1, index);
        index++;

        tokens_.emplace_back(Token(TokenType::QuotedString, ref));
        break;
      }
#if 0
      // utf16 = u"Hello, World
      case u8'u': {
        if ((index + 1) < text.length() && text[index + 1] == u8'"') {
          auto start = index + 1;
          index += 2;
          bool escaped = false;

          while (index < text.length() && (escaped || text[index] != u8'"')) {
            if (text[index] == u8'\r' || text[index] == u8'\n') {
              index++;
              break;
            }
            index++;
          }

          if (index == text.length() || text[index] != u8'"') {
            BASE_LOGE(kTag,("Unterminated string literal");
            return false; 
          }

          auto ref = make_ref(start + 1, index);
          index++;

          tokens_.emplace_back(Token(TokenType::QuotedStringU16, ref));
          break;
        }

        [[fallthrough]];
      }
      case u8'U': {
        if ((index + 1) < text.length() && text[index + 1] == u8'"') {
          auto start = index + 1;
          index += 2;
          bool escaped = false;

          while (index < text.length() && (escaped || text[index] != u8'"')) {
            if (text[index] == u8'\r' || text[index] == u8'\n') {
              index++;
              break;
            }
            index++;
          }

          if (index == text.length() || text[index] != u8'"') {
            BASE_LOGE(kTag,("Unterminated string literal");
            return false;
          }

          auto ref = make_ref(start + 1, index);
          index++;

          tokens_.emplace_back(Token(TokenType::QuotedStringU32, ref));
          break;
        }

		[[fallthrough]];
      }
#endif
      case u8'+': {
        auto start = index;
        index += 1;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            index += 1;
            tokens_.emplace_back(TokenType::PlusEquals, make_ref(start, start + 2));
            continue;
          } else if (text[index] == u8'+') {
            index += 1;
            tokens_.emplace_back(TokenType::PlusPlus, make_ref(start, start + 2));
            continue;
          }
        }
        tokens_.emplace_back(TokenType::Plus, make_ref(start, start + 1));
        break;
      }

      case u8'-': {
        auto start = index;
        index += 1;
        if (index < text.length()) {
          if (text[index] == u8'=') {
            index += 1;
            tokens_.emplace_back(TokenType::MinusEquals, make_ref(start, start + 2));
            continue;
          } else if (text[index] == u8'-') {
            index += 1;
            tokens_.emplace_back(TokenType::MinusMinus, make_ref(start, start + 2));
            continue;
          }
        }
        tokens_.emplace_back(TokenType::Minus, make_ref(start, start + 1));
        break;
      }

      case u8'*': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::AsteriskEqual, make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::Asterisk, make_ref(start, start + 1));
        break;
      }

      case u8'/': {
        auto start = index;
        index += 1;

        if (index < text.length()) {
          if (text[index] == u8'=') {
            index += 1;
            tokens_.emplace_back(TokenType::ForwardSlashEqual,
                                 make_ref(start, start + 2));
            continue;
          } else if (text[index] == u8'/') {
            // We are in a comment, skip it
            while (index < text.length()) {
              if (text[index] == u8'\n') {
                tokens_.emplace_back(TokenType::Comment, make_ref(start, index));
                index += 1;
                break;
              }
              index += 1;
            }
            continue;
          }
        }

        tokens_.emplace_back(TokenType::ForwardSlash, make_ref(start, start + 1));
        break;
      }

      case u8'=': {
        auto start = index;
        index += 1;

        if (index < text.length()) {
          if (text[index] == u8'=') {
            index += 1;
            tokens_.emplace_back(TokenType::DoubleEqual, make_ref(start, start + 2));
            continue;
          } else if (text[index] == u8'>') {
            index += 1;
            tokens_.emplace_back(TokenType::FatArrow, make_ref(start, start + 2));
            continue;
          }
        }

        tokens_.emplace_back(TokenType::Equal, make_ref(start, start + 1));
        break;
      }

      case u8'>': {
        auto start = index;
        index += 1;

        if (index < text.length()) {
          if (text[index] == u8'=') {
            index += 1;
            tokens_.emplace_back(TokenType::GreaterThanOrEqual,
                                 make_ref(start, start + 2));
            continue;
          } else if (text[index] == u8'>') {
            index += 1;
            if (index < text.length()) {
              if (text[index] == u8'=') {
                index += 1;
                tokens_.emplace_back(TokenType::RightShiftEqual,
                                     make_ref(start, start + 3));
              } else if (text[index] == u8'>') {
                index += 1;
                tokens_.emplace_back(TokenType::RightArithmeticShift,
                                     make_ref(start, start + 3));
              } else {
                index += 1;
                tokens_.emplace_back(TokenType::RightShift,
                                     make_ref(start, start + 2));
              }
            }

            continue;
          }
        }

        tokens_.emplace_back(TokenType::GreaterThan, make_ref(start, start + 1));
        break;
      }

      case u8'<': {
        auto start = index;
        index += 1;

        if (index < text.length()) {
          if (text[index] == u8'=') {
            index += 1;
            tokens_.emplace_back(TokenType::LessThanOrEqual,
                                 make_ref(start, start + 2));
            continue;
          } else if (text[index] == u8'>') {
            index += 1;
            if (index < text.length()) {
              if (text[index] == u8'=') {
                index += 1;
                tokens_.emplace_back(TokenType::LeftShiftEqual,
                                     make_ref(start, start + 3));
              } else if (text[index] == u8'>') {
                index += 1;
                tokens_.emplace_back(TokenType::LeftArithmeticShift,
                                     make_ref(start, start + 3));
              } else {
                index += 1;
                tokens_.emplace_back(TokenType::LeftShift,
                                     make_ref(start, start + 2));
              }
            }

            continue;
          }
        }

        tokens_.emplace_back(TokenType::LessThan, make_ref(start, start + 1));
        break;
      }

      case u8'!': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::NotEqual, make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::Asterisk, make_ref(start, start + 1));
        break;
      }

      case u8'&': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::AmpersandEqual,
                               make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::Ampersand, make_ref(start, start + 1));
        break;
      }

      case u8'|': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::PipeEqual, make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::Pipe, make_ref(start, start + 1));
        break;
      }

      case u8'^': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::CaretEqual, make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::Caret, make_ref(start, start + 1));
        break;
      }
      case u8'%': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::PercentSignEqual,
                               make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::PercentSign, make_ref(start, start + 1));
        break;
      }

      case u8'?': {
        auto start = index;
        index += 1;

        if ((index + 1) < text.length() && text[index] == u8'?' &&
            text[index + 1] == u8'=') {
          index += 1;
          tokens_.emplace_back(TokenType::QuestionMarkQuestionMarkEqual,
                               make_ref(start, start + 2));
          continue;
        } else if (index < text.length() && text[index] == u8'?') {
          index += 1;
          tokens_.emplace_back(TokenType::QuestionMarkQuestionMark,
                               make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::QuestionMark, make_ref(start, start + 1));
        break;
      }
      case u8'.': {
        auto start = index;
        index += 1;

        if (index < text.length() && text[index] == u8'.') {
          index += 1;
          tokens_.emplace_back(TokenType::DotDot, make_ref(start, start + 2));
          continue;
        }

        tokens_.emplace_back(TokenType::Dot, make_ref(start, start + 1));
        break;
      }
      default: {
        if (!LexItem(text, index))
            index++; // increment the index to avoid infinite loop and break out.
        break;
      }
    }
  }

  if (index < text.length())
    tokens_.emplace_back(TokenType::Eof, make_ref(index, index));

  return true;
}

bool Lexer::LexItem(const base::StringRefU8 text, mem_size& index) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  if (index >= text.length()) {
    return false;
  }

  // HACKFIX for now
  if (index == (text.length() - 1))
      return false;

  // prefixed number
  if (text[index] == u8'0' && index + 2 < text.length()) {
    switch (text[index + 1]) {
      case u8'x':
      case u8'X':
        return LexHexadecimalNumber(text, index);
      case u8'b':
      case u8'B':
        return LexBinaryNumber(text, index);
      case u8'o':
      case u8'O':
        return LexOctalNumber(text, index);
      default:
        break;
    }
  }

  // regular (decimal) number, without a prefix
  if (IsAsciiDigit(text[index]))
    return LexNumber(text, index);

  if (index >= text.length()) {
    return false;
  }

  // text or symbol name, those may start with an underscore
  else if (IsAsciiAlphabetic(text[index]) || text[index] == u8'_') {
    auto start = index;

    if ((index + 1) >= text.length()) {
      return false;
    }

    index += 1;
    if (index >= text.length()) {
	  return false;
	}

    bool is_escaped = false;
    while (index < text.length() && (IsAsciiAlphaNumeric(text[index])) ||
           text[index] == u8'_') {
      if (index == text.length()) {
		break;
	  }
      if (!is_escaped && text[index] == u8'\\') {
        is_escaped = true;
      } else
        is_escaped = false;
      index += 1;
    }

    tokens_.emplace_back(TokenType::CharacterSequence, make_ref(start, index));
  } else {
    BASE_LOGE(kTag, "Parse error: unknown character at index {}", index);
    // for now
    index++;
    return false;
  }

  return true;
}

bool Lexer::LexHexadecimalNumber(const base::StringRefU8 text, mem_size& index) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  auto start = index;
  index += 2;

  // ingest everything past 0x
  while (index < text.length() && IsAsciiHexDigit(text[index]) ||
         text[index] == u8'_' && text[index - 1] != u8'_') {
    index += 1;
  }

  // syntax error: number ending with underscore
  if (text[index - 1] == u8'_') {
    tokens_.emplace_back(TokenType::Unknown, make_ref(start, index));
    BASE_LOGE(kTag, "Hex numbers may not end with an underscore (at idx {})", index);
    return false;
  }

  if ((index - (start + 2)) == 0) {
    return false;
  }

  const auto text_number_slice = make_ref(start + 2, index);
  // auto number = BinToNumber(text_number_slice);

  tokens_.emplace_back(TokenType::HexNumber, text_number_slice);

  return true;
}

bool Lexer::LexOctalNumber(const base::StringRefU8 text, mem_size& index) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  auto start = index;
  index += 2;  // skip past 0b

  // skip over the whole numeric content that may only contain 1 or 0
  while (index < text.length() && (IsAsciiDigit(text[index]) &&
                                   text[index] != u8'8' && text[index] != u8'9') ||
         (text[index] == u8'_' && text[index - 1] != u8'_')) {
    index += 1;
  }

  // syntax error: number ending with underscore
  if (text[index - 1] == u8'_') {
    tokens_.emplace_back(TokenType::Unknown, make_ref(start, index));
    BASE_LOGE(kTag, "Octal numbers may not end with an underscore (at idx {})", index);
    return false;
  }

  if ((index - (start + 2)) == 0) {
    return false;
  }

  const auto text_number_slice = make_ref(start + 2, index);
  // auto number = BinToNumber(text_number_slice);

  tokens_.emplace_back(TokenType::OctalNumber, text_number_slice);
  return true;
}

bool Lexer::LexBinaryNumber(const base::StringRefU8 text, mem_size& index) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  auto start = index;
  index += 2;  // skip past 0b

  // skip over the whole numeric content that may only contain 1 or 0
  while (index < text.length() &&
         (text[index] == u8'0' || text[index] == u8'1' ||
          text[index] == u8'_' && text[index - 1] != u8'_')) {
    index += 1;
  }

  // syntax error: number ending with underscore
  if (text[index - 1] == u8'_') {
    tokens_.emplace_back(TokenType::Unknown, make_ref(start, index));
    BASE_LOGE(kTag, "Binary numbers may not end with an underscore (at idx {})", index);
    return false;
  }

  if ((index - (start + 2)) == 0) {
    return false;
  }

  const auto text_number_slice = make_ref(start + 2, index);
  // auto number = BinToNumber(text_number_slice);

  tokens_.emplace_back(TokenType::BinaryNumber, text_number_slice);

  return false;
}

bool Lexer::LexNumber(const base::StringRefU8 text, mem_size& index) {
  auto make_ref = [&](mem_size start, mem_size end) {
    return base::StringRefU8(&text.data()[start], end - start);
  };

  // Number
  bool is_floating_point = false;
  auto start = index;

  // skip over the whole numeric content
  // for floats, this returns as soon as the first . is hit, then we can examine the
  // rest.
  while (index < text.length() && IsAsciiDigit(text[index]) ||
         (text[index] == u8'_') && text[index - 1] != u8'_') {
    index += 1;
  }

  if (text[index - 1] == u8'_') {
    CHECK_BREAK;
  } else if (text[index] == u8'.' && IsAsciiDigit(text[index - 1])) {
    index += 1;
    while (index < text.length() && IsAsciiDigit(text[index]) ||
           (text[index] == u8'_') && text[index - 1] != u8'_' ||
           text[index] == u8'e' || text[index] == u8'E' || text[index] == u8'+') {
      index += 1;
    }

    is_floating_point = true;
  }

  // for now...
  if (is_floating_point) {
    tokens_.emplace_back(TokenType::FloatingNumber, make_ref(start, index));
    return true;
  }

  // deduce a type based on the numeric width, in case the type is assigned to a var
  // (auto) variable and so we can do bounds checking in the AST if we cannot fit the
  // number into the type
  const LiteralSuffix numeric_suffix = ConsumeNumericLiteralSuffix(text, index);
  if (numeric_suffix != LiteralSuffix::NONE) {
  }

  tokens_.emplace_back(TokenType::Number, make_ref(start, index));
  return true;
}
}  // namespace insane