// Copyright (C) 2022 Vincent Hengel.

#include "lexer.h"
#include <base/logging.h>  // for fmt
#include <gtest/gtest.h>

namespace {
using namespace aki;

static void DumpTokens(const base::Vector<aki::Token>& tok) {
  for (mem_size i = 0; i < tok.size(); ++i) {
    const auto& t = tok[i];
    fmt::print("idx: {}: {}: {}\n", i, t.StringifyType(),
               (const char*)t.StringifyContent().c_str());
  }
}

TEST(Lexer, TestSimpleTokens) {
  aki::Lexer lex;
  lex.Parse(u8";:+++=");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 5);

  EXPECT_EQ(tokens[0].type, TokenType::Semicolon);
  EXPECT_TRUE(tokens[0].value == u8";");

  EXPECT_EQ(tokens[1].type, TokenType::Colon);
  EXPECT_TRUE(tokens[1].value == u8":");

  EXPECT_EQ(tokens[2].type, TokenType::PlusPlus);
  EXPECT_TRUE(tokens[2].value == u8"++");

  EXPECT_EQ(tokens[3].type, TokenType::PlusEquals);
  EXPECT_TRUE(tokens[3].value == u8"+=");
}

TEST(Lexer, ParseNumberDecl) {
  aki::Lexer lex;
  lex.Parse(u8"f32 floating_point = 1.2345;");

  base::Vector<aki::Token>& tokens = lex.tokens();
  EXPECT_EQ(tokens.size(), 6);

  EXPECT_EQ(tokens[0].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[0].value == u8"f32");

  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[1].value == u8"floating_point");

  EXPECT_EQ(tokens[2].type, TokenType::Equal);
  EXPECT_TRUE(tokens[2].value == u8"=");

  EXPECT_EQ(tokens[3].type, TokenType::FloatNumber);
  EXPECT_TRUE(tokens[3].value == u8"1.2345");

  EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
  EXPECT_TRUE(tokens[4].value == u8";");

  EXPECT_EQ(tokens[5].type, TokenType::Eof);
  EXPECT_TRUE(tokens[5].value == u8"");
}

TEST(Lexer, OperatorTokens) {
  aki::Lexer lex;
  lex.Parse(u8"= == + ++ += - -- -= * *=");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 11);

  // Validate each operator token
  EXPECT_EQ(tokens[0].type, TokenType::Equal);
  EXPECT_EQ(tokens[1].type, TokenType::DoubleEqual);
  EXPECT_EQ(tokens[2].type, TokenType::Plus);
  EXPECT_EQ(tokens[3].type, TokenType::PlusPlus);
  EXPECT_EQ(tokens[4].type, TokenType::PlusEquals);
  EXPECT_EQ(tokens[5].type, TokenType::Minus);
  EXPECT_EQ(tokens[6].type, TokenType::MinusMinus);
  EXPECT_EQ(tokens[7].type, TokenType::MinusEquals);
  EXPECT_EQ(tokens[8].type, TokenType::Asterisk);
  EXPECT_EQ(tokens[9].type, TokenType::AsteriskEqual);
}

TEST(Lexer, NumberVariety) {
  aki::Lexer lex;
  lex.Parse(u8"123 123.456 0x12ab 0b1010 0o77 123u32 456 f32");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 9);

  // the lexer will strip the prefix in its internal repr
  EXPECT_EQ(tokens[0].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[0].value == u8"123");

  EXPECT_EQ(tokens[1].type, TokenType::FloatNumber);
  EXPECT_TRUE(tokens[1].value == u8"123.456");

  EXPECT_EQ(tokens[2].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[2].value == u8"12ab");

  EXPECT_EQ(tokens[3].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[3].value == u8"1010");

  EXPECT_EQ(tokens[4].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[4].value == u8"77");

  EXPECT_EQ(tokens[5].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[5].value == u8"123u32");

  EXPECT_EQ(tokens[6].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[6].value == u8"456");

  EXPECT_EQ(tokens[7].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[7].value == u8"f32");
}

TEST(Lexer, StringLiterals) {
  aki::Lexer lex;
  lex.Parse(
      u8"\"plain_string\" u8\"utf8_string\" u16\"utf16_string\" u32\"utf32_string\"");

  base::Vector<aki::Token>& tokens = lex.tokens();
  //DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 5);

  EXPECT_EQ(tokens[0].type, TokenType::QuotedString);
  EXPECT_TRUE(tokens[0].value == u8"plain_string");

  // while its redudant to do bc all strings are utf8, we allow to make it clear(er) to
  // the user
  EXPECT_EQ(tokens[1].type, TokenType::QuotedString);
  EXPECT_TRUE(tokens[1].value == u8"utf8_string");

  EXPECT_EQ(tokens[2].type, TokenType::QuotedStringU16);
  EXPECT_TRUE(tokens[2].value == u8"utf16_string");

  EXPECT_EQ(tokens[3].type, TokenType::QuotedStringU32);
  EXPECT_TRUE(tokens[3].value == u8"utf32_string");

  EXPECT_EQ(tokens[4].type, TokenType::Eof);
}

TEST(Lexer, Comments) {
  aki::Lexer lex;
  lex.Parse(u8"// Line comment\n/* Block comment */");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 3);  // Including Eol

  EXPECT_EQ(tokens[0].type, TokenType::LineComment);
  EXPECT_TRUE(tokens[0].value == u8"// Line comment");

  // not emitted on purpose.
  //  EXPECT_EQ(tokens[1].type, TokenType::Eol);

  EXPECT_EQ(tokens[1].type, TokenType::BlockComment);
  EXPECT_TRUE(tokens[1].value == u8"/* Block comment */");
}

TEST(Lexer, InvalidOrEdgeTokens) {
  aki::Lexer lex;
  lex.Parse(u8"123abc .123 ~= $# notinvalid");

  base::Vector<aki::Token>& tokens = lex.tokens();
  //DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 9);

  EXPECT_EQ(tokens[0].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[0].value == u8"123");

  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[1].value == u8"abc");

  EXPECT_EQ(tokens[2].type, TokenType::FloatNumber);
  EXPECT_TRUE(tokens[2].value == u8".123");

  EXPECT_EQ(tokens[3].type, TokenType::Tilde);
  EXPECT_TRUE(tokens[3].value == u8"~");
  EXPECT_EQ(tokens[4].type, TokenType::Equal);
  EXPECT_TRUE(tokens[4].value == u8"=");

  EXPECT_EQ(tokens[5].type, TokenType::Dollar);
  EXPECT_TRUE(tokens[5].value == u8"$");
  EXPECT_EQ(tokens[6].type, TokenType::Hash);
  EXPECT_TRUE(tokens[6].value == u8"#");

  EXPECT_EQ(tokens[7].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[7].value == u8"notinvalid");
}

TEST(Lexer, ColonColonAndFatArrow) {
  aki::Lexer lex;
  lex.Parse(u8"::=>");

  base::Vector<aki::Token>& tokens = lex.tokens();
  EXPECT_EQ(tokens.size(), 3);

  EXPECT_EQ(tokens[0].type, TokenType::ColonColon);
  EXPECT_TRUE(tokens[0].value == u8"::");

  EXPECT_EQ(tokens[1].type, TokenType::FatArrow);
  EXPECT_TRUE(tokens[1].value == u8"=>");
}

TEST(Lexer, NumberWithUnderscores) {
  aki::Lexer lex;
  lex.Parse(u8"1_234_567 0b1010_1010 0x12_ab_CD 0o7_77 1_234.56_78");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 6);

  EXPECT_EQ(tokens[0].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[0].value == u8"1_234_567");

  EXPECT_EQ(tokens[1].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[1].value == u8"1010_1010");

  EXPECT_EQ(tokens[2].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[2].value == u8"12_ab_CD");

  EXPECT_EQ(tokens[3].type, TokenType::IntegerNumber);
  EXPECT_TRUE(tokens[3].value == u8"7_77");

  EXPECT_EQ(tokens[4].type, TokenType::FloatNumber);
  EXPECT_TRUE(tokens[4].value == u8"1_234.56_78");
}

TEST(Lexer, ShiftOperators) {
  aki::Lexer lex;
  lex.Parse(u8"<< >> <<= >>= >>> <<<");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 7);

  EXPECT_EQ(tokens[0].type, TokenType::LeftShift);
  EXPECT_TRUE(tokens[0].value == u8"<<");

  EXPECT_EQ(tokens[1].type, TokenType::RightShift);
  EXPECT_TRUE(tokens[1].value == u8">>");

  EXPECT_EQ(tokens[2].type, TokenType::LeftShiftEqual);
  EXPECT_TRUE(tokens[2].value == u8"<<=");

  EXPECT_EQ(tokens[3].type, TokenType::RightShiftEqual);
  EXPECT_TRUE(tokens[3].value == u8">>=");

  EXPECT_EQ(tokens[4].type, TokenType::RightArithmeticShift);
  EXPECT_TRUE(tokens[4].value == u8">>>");

  EXPECT_EQ(tokens[5].type, TokenType::LeftArithmeticShift);
  EXPECT_TRUE(tokens[5].value == u8"<<<");
}

TEST(Lexer, DotOperators) {
  aki::Lexer lex;
  lex.Parse(u8". .. ...");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 4);

  EXPECT_EQ(tokens[0].type, TokenType::Dot);
  EXPECT_TRUE(tokens[0].value == u8".");

  EXPECT_EQ(tokens[1].type, TokenType::Range);
  EXPECT_TRUE(tokens[1].value == u8"..");

  EXPECT_EQ(tokens[2].type, TokenType::Ellipsis);
  EXPECT_TRUE(tokens[2].value == u8"...");
}

TEST(Lexer, ComparisonsAndEquality) {
  aki::Lexer lex;
  lex.Parse(u8"< <= > >= == != !");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 8);

  EXPECT_EQ(tokens[0].type, TokenType::LessThan);
  EXPECT_TRUE(tokens[0].value == u8"<");

  EXPECT_EQ(tokens[1].type, TokenType::LessThanOrEqual);
  EXPECT_TRUE(tokens[1].value == u8"<=");

  EXPECT_EQ(tokens[2].type, TokenType::GreaterThan);
  EXPECT_TRUE(tokens[2].value == u8">");

  EXPECT_EQ(tokens[3].type, TokenType::GreaterThanOrEqual);
  EXPECT_TRUE(tokens[3].value == u8">=");

  EXPECT_EQ(tokens[4].type, TokenType::DoubleEqual);
  EXPECT_TRUE(tokens[4].value == u8"==");

  EXPECT_EQ(tokens[5].type, TokenType::NotEqual);
  EXPECT_TRUE(tokens[5].value == u8"!=");

  EXPECT_EQ(tokens[6].type, TokenType::Not);
  EXPECT_TRUE(tokens[6].value == u8"!");
}

TEST(Lexer, AssignmentOperators) {
  aki::Lexer lex;
  lex.Parse(u8"= += -= *= /= %= &= |= ^= ~=");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 12);

  EXPECT_EQ(tokens[0].type, TokenType::Equal);
  EXPECT_TRUE(tokens[0].value == u8"=");

  EXPECT_EQ(tokens[1].type, TokenType::PlusEquals);
  EXPECT_TRUE(tokens[1].value == u8"+=");

  EXPECT_EQ(tokens[2].type, TokenType::MinusEquals);
  EXPECT_TRUE(tokens[2].value == u8"-=");

  EXPECT_EQ(tokens[3].type, TokenType::AsteriskEqual);
  EXPECT_TRUE(tokens[3].value == u8"*=");

  EXPECT_EQ(tokens[4].type, TokenType::ForwardSlashEqual);
  EXPECT_TRUE(tokens[4].value == u8"/=");

  EXPECT_EQ(tokens[5].type, TokenType::PercentSignEqual);
  EXPECT_TRUE(tokens[5].value == u8"%=");

  EXPECT_EQ(tokens[6].type, TokenType::AmpersandEqual);
  EXPECT_TRUE(tokens[6].value == u8"&=");

  EXPECT_EQ(tokens[7].type, TokenType::PipeEqual);
  EXPECT_TRUE(tokens[7].value == u8"|=");

  EXPECT_EQ(tokens[8].type, TokenType::CaretEqual);
  EXPECT_TRUE(tokens[8].value == u8"^=");

  EXPECT_EQ(tokens[9].type, TokenType::Tilde);
  EXPECT_TRUE(tokens[9].value == u8"~");  // ~= not defined, so ~ and = separate
}

TEST(Lexer, QuestionMarkOperators) {
  aki::Lexer lex;
  lex.Parse(u8"? ?? ?= ??= ?==");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 8);

  EXPECT_EQ(tokens[0].type, TokenType::QuestionMark);
  EXPECT_TRUE(tokens[0].value == u8"?");

  EXPECT_EQ(tokens[1].type, TokenType::NullishCoalescing);
  EXPECT_TRUE(tokens[1].value == u8"??");

  // ?= is not a valid operator in this context
  EXPECT_EQ(tokens[2].type, TokenType::QuestionMark);
  EXPECT_TRUE(tokens[2].value == u8"?");
  EXPECT_EQ(tokens[3].type, TokenType::Equal);
  EXPECT_TRUE(tokens[3].value == u8"=");

  EXPECT_EQ(tokens[4].type, TokenType::NullishCoalescingEqual);
  EXPECT_TRUE(tokens[4].value == u8"??=");

  // neither does this exist
  EXPECT_EQ(tokens[5].type, TokenType::QuestionMark);
  EXPECT_TRUE(tokens[5].value == u8"?");
  EXPECT_EQ(tokens[6].type, TokenType::DoubleEqual);
  EXPECT_TRUE(tokens[6].value == u8"==");
}

TEST(Lexer, MixedSymbolicIdentifiers) {
  aki::Lexer lex;
  lex.Parse(u8"_start _var123 $special @at at@ @");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 9);

  EXPECT_EQ(tokens[0].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[0].value == u8"_start");

  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[1].value == u8"_var123");

  EXPECT_EQ(tokens[2].type, TokenType::Dollar);
  EXPECT_TRUE(tokens[2].value == u8"$");

  EXPECT_EQ(tokens[3].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[3].value == u8"special");

  EXPECT_EQ(tokens[4].type, TokenType::Invalid);
  EXPECT_EQ(tokens[4].value,
            u8"@");  // Invalid because @ is not a valid identifier start

  EXPECT_EQ(tokens[5].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[5].value == u8"at");

  EXPECT_EQ(tokens[6].type, TokenType::Identifier);
  EXPECT_TRUE(tokens[6].value == u8"at@");

  EXPECT_EQ(tokens[7].type, TokenType::Invalid);
}

TEST(Lexer, EmptyInput) {
  aki::Lexer lex;
  lex.Parse(u8"");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].type, TokenType::Eof);
}

TEST(Lexer, OnlyWhitespace) {
  aki::Lexer lex;
  lex.Parse(u8"   \t\r\n\t");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // DumpTokens(tokens);
  //  Should contain only EOL from newline
  ASSERT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].type, TokenType::Eol);
  EXPECT_EQ(tokens[1].type, TokenType::Eof);
}

TEST(Lexer, UnterminatedString) {
  aki::Lexer lex;
  lex.Parse(u8"\"unterminated string");
  auto& tokens = lex.tokens();
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].type, TokenType::Invalid);
  EXPECT_EQ(tokens[1].type, TokenType::Eof);
 // DumpTokens(tokens);
//  EXPECT_FALSE(result);
}

}  // namespace
