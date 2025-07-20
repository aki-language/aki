// Copyright (C) The Fusion Authors/Vincent Hengel 2023-2025
#pragma once

#include <base/strings/string_ref.h>

namespace aki {
enum class TokenType {
  Invalid,
  InvalidNumber,
  Eol,           // End of line
  Eof,           // End of file
  Semicolon,     // ;
  Colon,         // :
  ColonColon,    // ::
  Comma,         // ,
  LParen,        // (
  RParen,        // )
  LCurly,        // {
  RCurly,        // }
  LSquare,       // [
  RSquare,       // ]
  Dollar,        // $
  Tilde,         // ~
  Hash,          // #
  Dot,           // .
  Plus,          // +
  Minus,         // -
  Asterisk,      // *
  ForwardSlash,  // /
  Equal,         // =
  GreaterThan,   // >
  LessThan,      // <
  Not,           // !
  Ampersand,     // &
  Pipe,          // |
  Caret,         // ^
  PercentSign,   // %
  QuestionMark,  // ?

  // Multi-character operators
  PlusPlus,                // ++
  PlusEquals,              // +=
  MinusMinus,              // --
  MinusEquals,             // -=
  AsteriskEqual,           // *=
  ForwardSlashEqual,       // /=
  DoubleEqual,             // ==
  FatArrow,                // =>
  Arrow,                   // ->
  GreaterThanOrEqual,      // >=
  LessThanOrEqual,         // <=
  NotEqual,                // !=
  AmpersandEqual,          // &=
  PipeEqual,               // |=
  CaretEqual,              // ^=
  PercentSignEqual,        // %=
  LogicalAnd,              // &&
  LogicalOr,               // ||
  NullishCoalescing,       // ??
  NullishCoalescingEqual,  // ??=
  QuestionDot,             // ?.

  // Shift operators
  LeftShift,             // <<
  RightShift,            // >>
  LeftShiftEqual,        // <<=
  RightShiftEqual,       // >>=
  LeftArithmeticShift,   // <<<
  RightArithmeticShift,  // >>>

  // Dot sequences
  Range,     // ..
  Ellipsis,  // ...

  // Literals (numbers are converted from hex, octal, binary, etc. to decimal)
  IntegerNumber,    // 123, 0x1F
  FloatNumber,      // 3.14, 1.2e3
  BoolLiteral,      // true, false
  QuotedString,     // "hello"
  QuotedStringU16,  // u"hello"
  QuotedStringU32,  // U"hello"
  TemplateLiteral,  // `hello ${world}`

  // Identifiers and keywords
  Identifier,
  FnKeyword,      // fn
  LetKeyword,     // let
  ConstKeyword,   // const
  IfKeyword,      // if
  ElseKeyword,    // else
  ForKeyword,     // for
  WhileKeyword,   // while
  ReturnKeyword,  // return

  // Comments
  LineComment,   // // ...
  BlockComment,  // /* ... */

  COUNT
};

enum class LiteralSuffix {
  NONE,
  UZ,
  U8,
  U16,
  U32,
  U64,
  I8,
  I16,
  I32,
  I64,
  F32,
  F64,
};

const char* TokenTypeToName(TokenType type) noexcept;

struct Token {
  explicit Token(TokenType t, base::StringRefU8 ref) : type(t), value(ref) {}

  TokenType type;
  base::StringRefU8 value;

  inline base::StringU8 StringifyContent() const {
    base::StringU8 token_content(value.data(), value.length() + 1);
    token_content.at(value.length()) = 0;
    return token_content;
  }

  inline const char* StringifyType() const { return TokenTypeToName(type); }
};
}  // namespace aki
