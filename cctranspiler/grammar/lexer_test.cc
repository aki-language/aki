// Copyright (C) 2022 Vincent Hengel.

#include "lexer.h"
#include <base/logging.h> // for fmt
#include <gtest/gtest.h>

namespace {
using namespace aki;

static void DumpTokens(const base::Vector<aki::Token>& tok)
{
   for (const auto& t : tok)
    fmt::print("{}: {}\n", (int)t.type, (const char*)t.StringifyContent().c_str());
}

TEST(Lexer, TestSimpleTokens) {
  aki::Lexer lex;
  lex.Parse(u8";:+++=");  // Tokens: ;, :, ++, +=

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 4);

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
  // Existing test remains unchanged
  aki::Lexer lex;
  lex.Parse(u8"f32 floating_point = 1.2345;");

  base::Vector<aki::Token>& tokens = lex.tokens();
  EXPECT_EQ(tokens.size(), 5);

  EXPECT_EQ(tokens[0].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[0].value == u8"f32");

  EXPECT_EQ(tokens[1].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[1].value == u8"floating_point");

  EXPECT_EQ(tokens[2].type, TokenType::Equal);
  EXPECT_TRUE(tokens[2].value == u8"=");

  EXPECT_EQ(tokens[3].type, TokenType::FloatingNumber);
  EXPECT_TRUE(tokens[3].value == u8"1.2345");

  EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
  EXPECT_TRUE(tokens[4].value == u8";");
}

TEST(Lexer, OperatorTokens) {
  aki::Lexer lex;
  lex.Parse(u8"= == + ++ += - -- -= * *=");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 10);

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
  ASSERT_EQ(tokens.size(), 9);

  // the lexer will strip the prefix in its internal repr
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_TRUE(tokens[0].value == u8"123");

  EXPECT_EQ(tokens[1].type, TokenType::FloatingNumber);
  EXPECT_TRUE(tokens[1].value == u8"123.456");

  EXPECT_EQ(tokens[2].type, TokenType::HexNumber);
  EXPECT_TRUE(tokens[2].value == u8"12ab");

  EXPECT_EQ(tokens[3].type, TokenType::BinaryNumber);
  EXPECT_TRUE(tokens[3].value == u8"1010");

  EXPECT_EQ(tokens[4].type, TokenType::OctalNumber);
  EXPECT_TRUE(tokens[4].value == u8"77");

  EXPECT_EQ(tokens[5].type, TokenType::Number);
  EXPECT_TRUE(tokens[5].value == u8"123");

  EXPECT_EQ(tokens[6].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[6].value == u8"u32");

  EXPECT_EQ(tokens[7].type, TokenType::Number);
  EXPECT_TRUE(tokens[7].value == u8"456");

  EXPECT_EQ(tokens[8].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[8].value == u8"f32");
}

TEST(Lexer, StringLiterals) {
  aki::Lexer lex;
  lex.Parse(
      u8"\"plain_string\" u8\"utf8_string\" u16\"utf16_string\" u32\"utf32_string\"");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 7);
  DumpTokens(tokens);

  EXPECT_EQ(tokens[0].type, TokenType::QuotedString);
  EXPECT_TRUE(tokens[0].value == u8"\"plain_string\"");

  EXPECT_EQ(tokens[1].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[1].value == u8"u8");

  EXPECT_EQ(tokens[2].type, TokenType::QuotedString);
  EXPECT_TRUE(tokens[2].value == u8"\"utf8_string\"");

  EXPECT_EQ(tokens[3].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[3].value == u8"u16");

  EXPECT_EQ(tokens[4].type, TokenType::QuotedStringU16);
  EXPECT_TRUE(tokens[4].value == u8"\"utf16_string\"");

  EXPECT_EQ(tokens[5].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[5].value == u8"u32");

  EXPECT_EQ(tokens[6].type, TokenType::QuotedStringU32);
  EXPECT_TRUE(tokens[6].value == u8"\"utf32_string\"");
}

TEST(Lexer, Comments) {
  aki::Lexer lex;
  lex.Parse(u8"// Line comment\n/* Block comment */");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 3);  // Including Eol

  EXPECT_EQ(tokens[0].type, TokenType::Comment);
  EXPECT_TRUE(tokens[0].value == u8"// Line comment");

  EXPECT_EQ(tokens[1].type, TokenType::Eol);

  EXPECT_EQ(tokens[2].type, TokenType::Comment);
  EXPECT_TRUE(tokens[2].value == u8"/* Block comment */");
}

TEST(Lexer, InvalidOrEdgeTokens) {
  aki::Lexer lex;
  lex.Parse(u8"123abc .123 ~= $# invalid");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 7);

  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_TRUE(tokens[0].value == u8"123");

  EXPECT_EQ(tokens[1].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[1].value == u8"abc");

  EXPECT_EQ(tokens[2].type, TokenType::Dot);
  EXPECT_TRUE(tokens[2].value == u8".");

  EXPECT_EQ(tokens[3].type, TokenType::FloatingNumber);
  EXPECT_TRUE(tokens[3].value == u8"123");

  EXPECT_EQ(tokens[4].type, TokenType::Tilde);
  EXPECT_TRUE(tokens[4].value == u8"~");

  EXPECT_EQ(tokens[5].type, TokenType::Equal);
  EXPECT_TRUE(tokens[5].value == u8"=");

  EXPECT_EQ(tokens[6].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[6].value == u8"$#");
}

TEST(Lexer, ColonColonAndFatArrow) {
  aki::Lexer lex;
  lex.Parse(u8"::=>");

  base::Vector<aki::Token>& tokens = lex.tokens();
  EXPECT_EQ(tokens.size(), 2);

  EXPECT_EQ(tokens[0].type, TokenType::ColonColon);
  EXPECT_TRUE(tokens[0].value == u8"::");

  EXPECT_EQ(tokens[1].type, TokenType::FatArrow);
  EXPECT_TRUE(tokens[1].value == u8"=>");
}

TEST(Lexer, NumberWithUnderscores) {
  aki::Lexer lex;
  lex.Parse(u8"1_234_567 0b1010_1010 0x12_ab_CD 0o7_77 1_234.56_78");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 5);

  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_TRUE(tokens[0].value == u8"1_234_567");

  EXPECT_EQ(tokens[1].type, TokenType::BinaryNumber);
  EXPECT_TRUE(tokens[1].value == u8"1010_1010");

  EXPECT_EQ(tokens[2].type, TokenType::HexNumber);
  EXPECT_TRUE(tokens[2].value == u8"12_ab_CD");

  EXPECT_EQ(tokens[3].type, TokenType::OctalNumber);
  EXPECT_TRUE(tokens[3].value == u8"7_77");

  EXPECT_EQ(tokens[4].type, TokenType::FloatingNumber);
  EXPECT_TRUE(tokens[4].value == u8"1_234.56_78");
}

TEST(Lexer, ShiftOperators) {
  aki::Lexer lex;
  lex.Parse(u8"<< >> <<= >>= >>> <<<");

  base::Vector<aki::Token>& tokens = lex.tokens();
  DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 6);

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
  ASSERT_EQ(tokens.size(), 3);

  EXPECT_EQ(tokens[0].type, TokenType::Dot);
  EXPECT_TRUE(tokens[0].value == u8".");

  EXPECT_EQ(tokens[1].type, TokenType::DotDot);
  EXPECT_TRUE(tokens[1].value == u8"..");

  EXPECT_EQ(tokens[2].type, TokenType::DotDotDot);
  EXPECT_TRUE(tokens[2].value == u8"...");
}

TEST(Lexer, ComparisonsAndEquality) {
  aki::Lexer lex;
  lex.Parse(u8"< <= > >= == != !");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 7);

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

  EXPECT_EQ(tokens[6].type, TokenType::ExclamationPoint);
  EXPECT_TRUE(tokens[6].value == u8"!");
}

TEST(Lexer, AssignmentOperators) {
  aki::Lexer lex;
  lex.Parse(u8"= += -= *= /= %= &= |= ^= ~=");

  base::Vector<aki::Token>& tokens = lex.tokens();
  ASSERT_EQ(tokens.size(), 10);

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
  ASSERT_EQ(tokens.size(), 5);

  EXPECT_EQ(tokens[0].type, TokenType::QuestionMark);
  EXPECT_TRUE(tokens[0].value == u8"?");

  EXPECT_EQ(tokens[1].type, TokenType::QuestionMarkQuestionMark);
  EXPECT_TRUE(tokens[1].value == u8"??");

  EXPECT_EQ(tokens[2].type, TokenType::QuestionMark);
  EXPECT_TRUE(tokens[2].value == u8"?");

  EXPECT_EQ(tokens[3].type, TokenType::Equal);
  EXPECT_TRUE(tokens[3].value == u8"=");

  EXPECT_EQ(tokens[4].type, TokenType::DoubleEqual);
  EXPECT_TRUE(tokens[4].value == u8"==");
}

TEST(Lexer, MixedSymbolicIdentifiers) {
  aki::Lexer lex;
  lex.Parse(u8"_start _var123 $special @at at@ @");

  base::Vector<aki::Token>& tokens = lex.tokens();
  DumpTokens(tokens);
  ASSERT_EQ(tokens.size(), 7);

  EXPECT_EQ(tokens[0].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[0].value == u8"_start");

  EXPECT_EQ(tokens[1].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[1].value == u8"_var123");

  EXPECT_EQ(tokens[2].type, TokenType::Dollar);
  EXPECT_TRUE(tokens[2].value == u8"$");

  EXPECT_EQ(tokens[3].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[3].value == u8"special");

  EXPECT_EQ(tokens[4].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[4].value == u8"at");

  EXPECT_EQ(tokens[5].type, TokenType::CharacterSequence);
  EXPECT_TRUE(tokens[5].value == u8"at@");

  EXPECT_EQ(tokens[6].type, TokenType::Unknown);
}

TEST(Lexer, EmptyInput) {
  aki::Lexer lex;
  lex.Parse(u8"");

  base::Vector<aki::Token>& tokens = lex.tokens();
  EXPECT_TRUE(tokens.empty());
}

TEST(Lexer, OnlyWhitespace) {
  aki::Lexer lex;
  lex.Parse(u8"   \t\r\n\t");

  base::Vector<aki::Token>& tokens = lex.tokens();
  // Should contain only EOL from newline
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].type, TokenType::Eol);
}

TEST(Lexer, UnterminatedString) {
  aki::Lexer lex;
  bool result = lex.Parse(u8"\"unterminated string");
  EXPECT_FALSE(result);
}

}  // namespace
