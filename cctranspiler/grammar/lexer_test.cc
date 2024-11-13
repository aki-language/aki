// Copyright (C) 2022 Vincent Hengel.

#include <gtest/gtest.h>
#include "lexer.h"

namespace {
using namespace insane;

TEST(Lexer, TestSimpleTokens) {}

TEST(Lexer, ParseNumber) {
  insane::Lexer lex;
  lex.Parse(u8"f32 floating_point = 1.2345;");

  auto& tokens = lex.tokens();
  EXPECT_EQ(tokens.size(), 6);
  #if 0
  EXPECT_EQ(tokens[0].type(), Token::Type::kIdentifier);
  EXPECT_EQ(tokens[0].text(), u8"f32");
  EXPECT_EQ(tokens[1].type(), Token::Type::kIdentifier);
  EXPECT_EQ(tokens[1].text(), u8"floating_point");
  EXPECT_EQ(tokens[2].type(), Token::Type::kOperator);
  EXPECT_EQ(tokens[2].text(), u8"=");
  EXPECT_EQ(tokens[3].type(), Token::Type::kNumber);
  EXPECT_EQ(tokens[3].text(), u8"1.2345");
  EXPECT_EQ(tokens[4].type(), Token::Type::kOperator);
  EXPECT_EQ(tokens[4].text(), u8";");
  #endif
}
}  // namespace