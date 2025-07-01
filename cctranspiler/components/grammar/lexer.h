// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <base/arch.h>
#include <base/containers/vector.h>
#include <base/strings/string_ref.h>
#include <base/strings/xstring.h>

#include "token.h"

namespace aki {

// The lexer splits the input stream into tokens.
class Lexer {
 public:
  Lexer() = default;
  ~Lexer() = default;

  bool Parse(const base::StringRefU8 text);

  auto& tokens() { return tokens_; }

 private:
  bool LexHexadecimalNumber(const base::StringRefU8, mem_size&);
  bool LexOctalNumber(const base::StringRefU8, mem_size&);
  bool LexBinaryNumber(const base::StringRefU8, mem_size&);
  bool LexNumber(const base::StringRefU8 text,
                 mem_size& index,
                 mem_size custom_start,
                 bool has_leading_dot);

  void LexIdentifier(const base::StringRefU8 text, mem_size& index);

 private:
  base::Vector<Token> tokens_;
};
}  // namespace aki
