// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <base/strings/string_ref.h>

namespace insane {
enum class TokenType {
  Unknown,
  Semicolon,
  Colon,
  ColonColon,
  Plus,
  PlusPlus,
  PlusEquals,
  Minus,
  MinusMinus,
  MinusEquals,
  AsteriskEqual,
  Asterisk,
  ForwardSlash,
  ForwardSlashEqual,
  Equal,
  DoubleEqual,
  FatArrow,
  GreaterThanOrEqual,
  RightShiftEqual,
  RightArithmeticShift,
  RightShift,
  GreaterThan,
  LessThanOrEqual,
  LeftShiftEqual,
  LeftArithmeticShift,
  LeftShift,
  LessThan,
  NotEqual,
  ExclamationPoint,
  AmpersandEqual,
  Ampersand,
  PipeEqual,
  Pipe,
  CaretEqual,
  Caret,
  Dollar,
  Tilde,
  Hash,
  PercentSignEqual,
  PercentSign,
  QuestionMarkQuestionMarkEqual,
  QuestionMarkQuestionMark,
  QuestionMark,
  Comma,
  Dot,
  DotDot,
  LParen,
  RParen,
  LCurly,
  RCurly,
  LSquare,
  RSquare,
  Eol,
  Eof,
  QuotedString,
  QuotedStringU16,
  QuotedStringU32,
  CharacterSequence,
  Comment,

  Number,
  FloatingNumber,
  BinaryNumber,
  HexNumber,
  OctalNumber,

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
    // as the token points to a range within an original source file, we have to copy
    // it to append a null terminator
    base::StringU8 token_content(value.data(), value.length() + 1);
    token_content.at(value.length()) = 0;

    return token_content;
  }

  inline const char* StringifyType() const { return TokenTypeToName(type); }
};
}  // namespace insane