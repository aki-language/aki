// Copyright (C) Vincent Hengel 2023-2025
#pragma once

#include <base/expected.h>
#include <base/optional.h>

#include "keyword.h"
#include "token.h"
#include "translation_unit.h"
#include "lexer.h"

#include "api.h"

namespace aki {
enum class ParseErrorCode {
  Unknown = 0,
  Success,
  UnknownTopLevelDeclaration,
  UnexpectedToken,
  UnexpectedEnding,
  ExpectedKeywordFunc,
  ExpectedIdentifier,
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

  NImpld,
};

struct ParseError {
  ParseErrorCode code;
  mem_size token_index;  // Index of the token that caused the error

  ParseError(ParseErrorCode code, mem_size index) : code(code), token_index(index) {}
};

struct ParseState {
  mem_size index = 0;  // Current position into the token array.
};
static_assert(sizeof(ParseState) == 8);

template <typename T>
struct ParseResult {
  using value_type = T;

  bool success;
  T value;                // The parsed value (e.g., a handle) if successful
  ParseError error;       // The error details if failed
  ParseState next_state;  // The state of the parser AFTER this operation

  static ParseResult<T> Ok(T val, ParseState state) {
    return {true, std::move(val), {ParseErrorCode::Success, state.index}, state};
  }
  static ParseResult<T> Err(ParseErrorCode code,
                            ParseState original_state,
                            T val_construc = {}) {
    return {false, std::move(val_construc), {code, original_state.index}, original_state};
  }

  // Allow checking in an if-statement
  operator bool() const { return success; }
};

// Specialization for void, for functions that don't return a value but advance state.
template <>
struct ParseResult<void> {
  bool success;
  ParseError error;
  ParseState next_state;

  static ParseResult<void> Ok(ParseState state) {
    return {true, {ParseErrorCode::Success, state.index}, state};
  }

  static ParseResult<void> Err(ParseErrorCode code, ParseState original_state) {
    return {false, {code, original_state.index}, original_state};
  }

  operator bool() const { return success; }
};

struct StateResult {
  ParseError err;
  mem_size update_idx;

  const bool has_changes(ParseState& ps) const { return update_idx != ps.index; }
};

// Consumes the current token if it matches the expected type.
// Returns a success result with the new state, or a failure result.
AKI_GRAMMAR_API ParseResult<void> ConsumeToken(const TokenList& tl,
                                               ParseState state,
                                               TokenType expected_type);

// Takes a TokenList produced by the lexer and builds our custom AST representation
AKI_GRAMMAR_API ParseError ParseTranslationUnit(const aki::TokenList& tokens,
                                                TranslationUnit& tu);

AKI_GRAMMAR_API ParseResult<void> ParseTopLevelDeclaration(const TokenList& tl,
                                                           TranslationUnit& ast,
                                                           ParseState current_state);
}  // namespace aki
