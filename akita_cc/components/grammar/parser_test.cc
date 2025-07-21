// Copyright (C) 2022 Vincent Hengel.

#include <gtest/gtest.h>

#include "parser.h"

namespace {
using namespace aki;

TEST(Parser, TestEmptyTokenList) {
  TranslationUnit tu;
  TokenList tl;

  auto r = ParseTranslationUnit(tl, tu);
  EXPECT_EQ(r.code, ParseErrorCode::Success);
}

TEST(Parser, TestIgnoreTrivia) {
  TranslationUnit tu;
  TokenList tl;
  tl.emplace_back(aki::TokenType::LineComment, u8"// My Line Comment");
  tl.emplace_back(aki::TokenType::Semicolon, u8";");
  tl.emplace_back(aki::TokenType::BlockComment, u8"/* My Block Comment */");
  tl.emplace_back(aki::TokenType::Eol, u8"\n");

  EXPECT_EQ(ParseTranslationUnit(tl, tu).code, ParseErrorCode::Success);

  EXPECT_TRUE(tu.current_scope()->all_empty());

  // The parser should ignore these tokens and not fail.
  // EXPECT_TRUE(tu.scopes.empty());
}
}  // namespace
