// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#if 0

#include <base/expected.h>
#include <base/optional.h>

#include "keyword.h"
#include "syntax.h"
#include "token.h"
#include "translation_unit.h"

namespace aki {
enum class ParseError {
  Unknown = 0,
  Success,
  UnexpectedToken,
  UnexpectedEnding,
  UnknownKeyword,
  MissingBrace,
  MissingSemicolon,
  MissingFunctionName,
  MissingSymbolName,
  MissingType,
  MissingColon,
  MissingComma,
  InvalidTypeDecleration,
  MissingLeftParenthesis,
  MissingRightParenthesis,
  MissingAssignment,
  InvalidOperand,
  InvalidExpression,

  // functions
  MissingParams,
  InvalidName,

  MissingParent,

  // Variables
  Missing,
  InvalidInbuiltType,
};

template <typename T = obj_handle>
struct ParseResult {
  using handle_type = T;

  ParseError status;
  handle_type maybe_handle;

  // empty constructor
  ParseResult()
      : status(ParseError::Unknown),
        maybe_handle((handle_type)TranslationUnit::invalid_handle) {}

  ParseResult(const ParseError status, handle_type maybe_handle = (handle_type)
                                           TranslationUnit::invalid_handle)
      : status(status), maybe_handle(maybe_handle) {}

  ParseResult(handle_type sure_handle)
      : status(ParseError::Success), maybe_handle(sure_handle) {}

  operator bool() const { return status == ParseError::Success; }

  // copy constructor (we allow copying of parse results for now, but we might)
  ParseResult(const ParseResult& other)
      : status(other.status), maybe_handle(other.maybe_handle) {}
};

// NOTE(Vince): The parser is responsible for taking a stream of tokens and
// turning them into syntax objects that can be used by codegen.
class Parser {
 public:
  Parser() = delete;
  explicit Parser(base::Vector<Token>);
  ~Parser() {};

  void ParseTokens();

  auto& translation_unit() { return objects_; }

 private:
  // parse subs
  ParseError ParseNamespace();
  ParseError ParseVisiblityDecleration(bool is_public);
  ParseResult<ParsedEnumDeclRef> ParseEnumDecleration();

  // a complex can be either a class or struct in our nomenclature
  ParseError ParseComplexDecleration(bool is_public);
  ParseError ParseAliasDeclerationl();

  ParseError ParseVariableDecleration(bool is_const);
  ParseError ParseParameterDecleration(ParsedParameterDeclRef&);
  ParseResult<ParsedFunctionDeclRef> ParseFunctionDecleration();

  ParseResult<ParsedStatementRef> ParseReturnStatement();
  ParseError ParseFunctionBody();

  ParseError ParseImport();
  ParseError ParseExternDecl();

  ParseError ConsumeKeyword(const KeywordType, base::Optional<bool>);

  ParseResult<ParsedStatementRef> ParseStatement2();
  ParseResult<ParsedExpressionRef> ParseExpression(bool has_assignment,
                                                   bool has_ptr);

  ParseResult<ParsedOpRef> ParseOperand(const bool ptr);

  base::StringRefU8 ParseCharacterSequence();
  base::StringRefU8 ParseString();

  // increments the current index, if the token is of the expected type.
  bool CheckForToken(TokenType type);

  ParseResult<ParsedTypeRef> ParseType();

 private:
  KeywordType EatKeyword();

  Token& Peek() const { return tokens_[current_index_]; }
  bool IsAtEnd() const { return Peek().type == TokenType::Eof; }

  // ref tracking into the original token array.
  base::Vector<Token> tokens_;
  mem_size current_index_{0};
  // int current_{0};

  // for now
  aki::TranslationUnit objects_;
};
}  // namespace aki

#endif