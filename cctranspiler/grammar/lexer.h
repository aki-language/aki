// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <base/arch.h>
#include <base/containers/vector.h>
#include <base/strings/string_ref.h>
#include <base/strings/xstring.h>

#include "token.h"

namespace insane {

// The lexer splits the input stream into tokens.
class Lexer {
 public:
  Lexer() = default;
  ~Lexer() = default;

  bool Parse(const base::StringRefU8 text);

  auto& tokens() { return tokens_; }

 private:
  bool LexItem(const base::StringRefU8 tex, mem_size& index);

  bool LexHexadecimalNumber(const base::StringRefU8, mem_size&);
  bool LexOctalNumber(const base::StringRefU8, mem_size&);
  bool LexBinaryNumber(const base::StringRefU8, mem_size&);
  bool LexNumber(const base::StringRefU8, mem_size&);

 private:
  base::Vector<Token> tokens_;
};
}  // namespace insane
