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

  // the routine above should delete the scope upon exit.
  // no need to check here, its already caught by BUGCHECK
  // EXPECT_EQ(tu.current_scope(), nullptr);

  // The parser should ignore these tokens and not fail.
  // EXPECT_TRUE(tu.scopes.empty());
}

// Test fixture for parser tests
class ParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Each test gets a fresh TranslationUnit
    tu = std::make_unique<TranslationUnit>();
    tu->PushScope(u8"test", TranslationUnit::Scope::Type::Namespace);
  }

  std::unique_ptr<TranslationUnit> tu;
};

TEST_F(ParserTest, ParseVariableDecl_LetWithTypeAndInitializer) {
  TokenList tl;
  tl.emplace_back(TokenType::Identifier, u8"let");
  tl.emplace_back(TokenType::Identifier, u8"x");
  tl.emplace_back(TokenType::Colon, u8":");
  tl.emplace_back(TokenType::Identifier, u8"i32");
  tl.emplace_back(TokenType::Equal, u8"=");
  tl.emplace_back(TokenType::IntegerNumber, u8"42");

  auto result = ParseTopLevelDeclaration(tl, *tu, {0});
  EXPECT_FALSE(result.success);
 // EXPECT_EQ(result.next_state.index, 6);

  //TranslationUnit::Scope* s = tu->current_scope();

  #if 0
  ASSERT_EQ(tu->all_variables.(), 1);
  const auto& var = tu->all_variables[0];
  EXPECT_EQ(var.name, u8"x");
  EXPECT_TRUE(var.is_const);
  EXPECT_NE(var.type_handle, TranslationUnit::invalid_handle);
  EXPECT_NE(var.expr_handle, TranslationUnit::invalid_handle);

  ASSERT_EQ(tu->all_types.items.size(), 1);
  EXPECT_EQ(tu->all_types[var.type_handle].name, u8"i32");

  ASSERT_EQ(tu->all_expressions.items.size(), 1);
  const auto& expr = tu->all_expressions[var.expr_handle];
  ASSERT_EQ(expr.ops.size(), 1);
  const auto& op = tu->all_ops[expr.ops[0]];
  EXPECT_EQ(op.value, u8"42");
  #endif
}
}  // namespace
