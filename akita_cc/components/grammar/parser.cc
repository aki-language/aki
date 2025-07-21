// Copyright (C) Vincent Hengel 2023-2025

#include "parser.h"

// Note(Vince):
// The aki parser deviates from the classical approach of building a pointer-based
// Abstract Syntax Tree (AST). Instead, it employs a data-oriented design
// inspired by Entity-Component-System (ECS) architectures, which is common in
// high-performance applications like game engines.
//
// The core idea is to store all AST nodes of the same type together in
// contiguous arrays (a "struct-of-arrays" layout). For example, there's one
// array for all function declarations, another for all variable declarations, etc.
//
// Nodes do not hold raw pointers to each other. Instead, they store type-safe,
// ID-based handles. A handle is essentially an index into one of the typed
// arrays.
//
// Key Advantages of this Approach:
//
// 1. Cache-Friendliness: Contiguous storage leads to excellent cache locality.
//    Iterating over all nodes of a specific type (e.g., type-checking all
//    functions) is extremely fast as the data is packed together in memory.
//
// 2. Stable References: ID-based handles are stable. They remain valid even if
//    the underlying arrays are reallocated, which completely eliminates the
//    dangling pointer problem common in traditional ASTs.
//
// 3. Resilience & Tombstoning: If a node becomes invalid (e.g., due to a
//    semantic error), its ID can be marked as "dead" or "tombstoned". Any
//    subsequent attempt to resolve this handle will safely fail instead of
//    accessing invalid memory.
//
// 4. Parallelism & Concurrency: The data layout is inherently friendly to
//    parallel processing. Different compiler passes can operate on slices of
//    the arrays concurrently with fewer data dependency hazards.
//
// 5. Simplified Serialization: An ID-based AST is trivial to serialize to
//    disk and load back, as there are no pointers to "swizzle" (i.e., fix up
//    after loading into a new memory space).
//

// In regards to this parsing setup, we use mostly a functional approach. If the function
// parsing succeeds, we "commit" a new state index pointer, if not we "revert"

namespace aki {
namespace {
constexpr char kLogTag[] = "langparser";

// Tries to parse. On success, it updates 'handle' with the value and 'current_state' with
// the new state. On failure, it immediately returns the failed result from the current
// function.
// For use in a function that returns ParseResult<T> where T is NOT void.
#define TRY_WITH_VALUE(handle, expr)                                             \
  do {                                                                           \
    auto result = (expr);                                                        \
    if (!result.success) {                                                       \
      /* Construct a failure result of the *same type* as the parent function */ \
      return {false, {}, result.error, result.next_state};                       \
    }                                                                            \
    (handle) = result.value;                                                     \
    current_state = result.next_state;                                           \
  } while (0)

// Use inside a function returning ParseResult<T> (where T is NOT void).
// Calls an expression that returns ParseResult<void>.
#define TRY_VOID(expr)                                                             \
  do {                                                                             \
    auto result = (expr);                                                          \
    if (!result.success) {                                                         \
      /* Returns a failure of the parent function's type, with a default value. */ \
      return {false, {}, result.error, result.next_state};                         \
    }                                                                              \
    current_state = result.next_state;                                             \
  } while (0)


// For use in a function that returns ParseResult<void>.
#define TRY_INTO_VOID(handle, expr)                              \
  do {                                                           \
    auto result = (expr);                                        \
    if (!result.success) {                                       \
      /* Construct a failure result of type ParseResult<void> */ \
      return {false, result.error, result.next_state};           \
    }                                                            \
    (handle) = result.value;                                     \
    current_state = result.next_state;                           \
  } while (0)

#define TRY_VOID_INTO_VOID(expr)                       \
  do {                                                 \
    auto result = (expr);                              \
    if (!result.success) {                             \
      return {false, result.error, result.next_state}; \
    }                                                  \
    current_state = result.next_state;                 \
  } while (0)

inline bool IsAtEnd(const TokenList& tl, const ParseState& state) {
  return state.index >= tl.size();
}

inline const Token* Peek(const TokenList& tl, ParseState state, size_t offset = 0) {
  if (state.index + offset >= tl.size())
    return nullptr;
  return &tl[state.index + offset];
}


inline void BumpIndex(ParseState& state, size_t count = 1) {
  state.index += count;
}

ParseState ConsumeTrivia(const TokenList& tl, ParseState state) {
  while (const Token* t = Peek(tl, state)) {
    switch (t->type) {
      case TokenType::LineComment:
      case TokenType::BlockComment:
      case TokenType::Eol:
      case TokenType::Semicolon:
        state.index++;
        continue;
      default:
        return state;
    }
  }
  return state;
}
}  // namespace

// Consumes the current token if it matches the expected type.
// Returns a success result with the new state, or a failure result.
ParseResult<void> ConsumeToken(const TokenList& tl,
                               ParseState state,
                               TokenType expected_type) {
  const Token* t = Peek(tl, state);
  if (!t) {
    return ParseResult<void>::Err(ParseErrorCode::UnexpectedEnding, state);
  }
  if (t->type != expected_type) {
    return ParseResult<void>::Err(ParseErrorCode::UnexpectedToken, state);
  }
  state.index++;
  return ParseResult<void>::Ok(state);
}

// Consumes the current token if it's an identifier.
// Returns the identifier's string value, or a failure result.
ParseResult<base::StringRefU8> ParseIdentifier(const TokenList& tl, ParseState state) {
  const Token* t = Peek(tl, state);
  if (!t) {
    return ParseResult<base::StringRefU8>::Err(ParseErrorCode::UnexpectedEnding, state,
                                               base::StringRefU8::null_ref());
  }
  if (t->type != TokenType::Identifier) {
    return ParseResult<base::StringRefU8>::Err(ParseErrorCode::ExpectedIdentifier, state,
                                               base::StringRefU8::null_ref());
  }
  state.index++;
  return ParseResult<base::StringRefU8>::Ok(t->value, state);
}

ParseResult<ParsedTypeRef> ParseType(const TokenList& tl,
                                     TranslationUnit& ast,
                                     ParseState current_state) {
  base::StringRefU8 type_name = base::StringRefU8::null_ref();
  TRY_WITH_VALUE(type_name, ParseIdentifier(tl, current_state));

  // For now, all types are just stored by name. A real implementation would check
  // for builtin types, pointers (&), arrays ([]), etc.
  auto type = ast.all_types.Create(ParsedType::Type::Name, type_name);
  return ParseResult<ParsedTypeRef>::Ok(type.handle, current_state);
}

ParseResult<ParsedParameterDeclRef> ParseParameterDeclaration(const TokenList& tl,
                                                              TranslationUnit& ast,
                                                              ParseState current_state) {
  base::StringRefU8 param_name = base::StringRefU8::null_ref();
  TRY_WITH_VALUE(param_name, ParseIdentifier(tl, current_state));

  TRY_VOID(ConsumeToken(tl, current_state, TokenType::Colon));

  ParsedTypeRef type_handle;
  TRY_WITH_VALUE(type_handle, ParseType(tl, ast, current_state));

  // In the old code, ParsedVariableDecl was created here. We'll simplify and
  // assume ParsedParameterDecl can be created directly or via a helper. For
  // this migration, we'll create the underlying variable decl first.
  ParsedVariableDecl var_decl(param_name, false, /* is_const */
                              Linkage::Internal, Visibility::Private, type_handle,
                              (ParsedExpressionRef)TranslationUnit::invalid_handle);

  auto param = ast.all_parameters.Create(base::move(var_decl), true);
  return ParseResult<ParsedParameterDeclRef>::Ok(param.handle, current_state);
}

ParseResult<ParsedFunctionDeclRef> ParseFunctionDeclaration(const TokenList& tl,
                                                            TranslationUnit& ast,
                                                            ParseState current_state) {
  // 1. Consume "func" keyword - This is already done by the dispatcher.
  // The state we receive is *after* the "func" keyword.
  current_state.index++;

  // 2. Parse function name
  base::StringRefU8 func_name = base::StringRefU8::null_ref();
  TRY_WITH_VALUE(func_name, ParseIdentifier(tl, current_state));

  // 3. Parse parameters
  TRY_VOID(ConsumeToken(tl, current_state, TokenType::LParen));

  base::Vector<ParsedParameterDeclRef> params;
  // Check if there are any parameters before entering the loop
  if (Peek(tl, current_state) && Peek(tl, current_state)->type != TokenType::RParen) {
    while (true) {
      ParsedParameterDeclRef param_handle;
      TRY_WITH_VALUE(param_handle, ParseParameterDeclaration(tl, ast, current_state));
      params.push_back(param_handle);

      // After a parameter, we expect a comma or a closing parenthesis
      const Token* next_token = Peek(tl, current_state);
      if (next_token && next_token->type == TokenType::RParen) {
        break;  // End of parameter list
      }
      TRY_VOID(ConsumeToken(tl, current_state, TokenType::Comma));
    }
  }

  TRY_VOID(ConsumeToken(tl, current_state, TokenType::RParen));

  // 4. Parse optional return type
  ParsedTypeRef return_type_handle = (ParsedTypeRef)TranslationUnit::invalid_handle;
  if (Peek(tl, current_state) && Peek(tl, current_state)->type == TokenType::Colon) {
    current_state.index++;  // Consume ':'
    TRY_WITH_VALUE(return_type_handle, ParseType(tl, ast, current_state));
  }

  // 5. Check for function body or forward declaration
  if (Peek(tl, current_state) && Peek(tl, current_state)->type == TokenType::Semicolon) {
    // Forward declaration
    current_state.index++;  // Consume ';'
    auto func = ast.all_functions.Create(func_name, params, return_type_handle,
                                         Linkage::External, Visibility::Public);
    return ParseResult<ParsedFunctionDeclRef>::Ok(func.handle, current_state);
  }

  TRY_VOID(ConsumeToken(tl, current_state, TokenType::LCurly));

  // 6. Parse function body (omitted for brevity, but would involve parsing statements)
  // For now, we'll just skip to the closing brace.
  // A real implementation would loop, calling ParseStatement() until '}'.
  while (Peek(tl, current_state) && Peek(tl, current_state)->type != TokenType::RCurly) {
    // TODO: Implement ParseStatement and call it here.
    current_state.index++;
  }

  TRY_VOID(ConsumeToken(tl, current_state, TokenType::RCurly));

  // 7. Create the function declaration in the AST
  auto func = ast.all_functions.Create(func_name, params, return_type_handle,
                                       Linkage::Internal, Visibility::Public);

  // 8. Return the handle and the new state
  return ParseResult<ParsedFunctionDeclRef>::Ok(func.handle, current_state);
}

ParseResult<ParsedExpressionRef> ParseExpression(const TokenList& tl,
                                                 TranslationUnit& ast,
                                                 ParseState current_state) {
  const Token* t = Peek(tl, current_state);
  if (!t) {
    return ParseResult<ParsedExpressionRef>::Err(ParseErrorCode::UnexpectedEnding,
                                                 current_state);
  }

  ParsedOpRef op_handle;
  switch (t->type) {
    case TokenType::IntegerNumber:
    case TokenType::FloatNumber: {
      auto op = ast.all_ops.Create(ParsedOp::Type::NumericConstant, ParsedOp::Flags::None,
                                   t->value);
      op_handle = op.handle;
      current_state.index++;
      break;
    }
    // TODO: Handle other operand types like identifiers, function calls, strings, etc.
    default:
      return ParseResult<ParsedExpressionRef>::Err(ParseErrorCode::InvalidExpression,
                                                   current_state);
  }

  // An expression is a list of operands/operators. For this simple case,
  // it's just one operand.
  base::Vector<ParsedOpRef> ops;
  ops.push_back(op_handle);

  auto expr = ast.all_expressions.Create(base::move(ops));
  return ParseResult<ParsedExpressionRef>::Ok(expr.handle, current_state);
}


ParseResult<ParsedVariableDeclRef> ParseVariableDeclaration(const TokenList& tl,
                                                            TranslationUnit& ast,
                                                            ParseState current_state,
                                                            bool is_const) {
  // let/var keyword is already consumed.
  current_state.index++;

  base::StringRefU8 var_name = base::StringRefU8::null_ref();
  TRY_WITH_VALUE(var_name, ParseIdentifier(tl, current_state));

  ParsedTypeRef type_handle = (ParsedTypeRef)TranslationUnit::invalid_handle;
  ParsedExpressionRef expr_handle = (ParsedExpressionRef)TranslationUnit::invalid_handle;

  // Check for explicit type annotation (e.g., `: i32`)
  if (Peek(tl, current_state) && Peek(tl, current_state)->type == TokenType::Colon) {
    current_state.index++;  // Consume ':'
    TRY_WITH_VALUE(type_handle, ParseType(tl, ast, current_state));
  }

  // Check for assignment (e.g., `= 42`)
  if (Peek(tl, current_state) && Peek(tl, current_state)->type == TokenType::Equal) {
    current_state.index++;  // Consume '='
    TRY_WITH_VALUE(expr_handle, ParseExpression(tl, ast, current_state));
  }

  if (type_handle == TranslationUnit::invalid_handle &&
      expr_handle == TranslationUnit::invalid_handle) {
    // Neither a type nor an initializer was provided.
    return ParseResult<ParsedVariableDeclRef>::Err(ParseErrorCode::InvalidTypeDecleration,
                                                   current_state);
  }

  auto var = ast.all_variables.Create(var_name, is_const, Linkage::Internal,
                                      Visibility::Private, type_handle, expr_handle);

  return ParseResult<ParsedVariableDeclRef>::Ok(var.handle, current_state);
}

// This is the main dispatcher for any top-level declaration.
// It tries to parse different constructs (functions, variables, etc.)
// in order.
ParseResult<void> ParseTopLevelDeclaration(const TokenList& tl,
                                           TranslationUnit& ast,
                                           ParseState current_state) {
  const Token* t = Peek(tl, current_state);
  if (!t) {
    // End of file is not an error here. It's just nothing to parse.
    // Returning success with the same state is correct.
    return ParseResult<void>::Ok(current_state);
  }

  if (t->type != TokenType::Identifier) {
    return ParseResult<void>::Err(ParseErrorCode::UnexpectedToken, current_state);
  }

  KeywordType kwd = MatchKeyword(t->value);
  TranslationUnit::Scope* cs = ast.current_scope();

  switch (kwd) {
    case KeywordType::Func: {
      ParsedFunctionDeclRef handle;
      TRY_INTO_VOID(handle, ParseFunctionDeclaration(tl, ast, current_state));
      cs->functions.emplace_back(handle);
      break;
    }
    case KeywordType::Let: {
      ParsedVariableDeclRef handle;
      TRY_INTO_VOID(handle,
                    ParseVariableDeclaration(tl, ast, current_state, /*is_const=*/true));
      cs->AddObject(TU::ObjectType::Variable, handle);
      break;
    }
    case KeywordType::Var: {
      ParsedVariableDeclRef handle;
      TRY_INTO_VOID(handle,
                    ParseVariableDeclaration(tl, ast, current_state, /*is_const=*/false));
      cs->AddObject(TU::ObjectType::Variable, handle);
      break;
    }
    default:
      return ParseResult<void>::Err(ParseErrorCode::UnknownTopLevelDeclaration,
                                    current_state);
  }

  // If we get here, one of the cases succeeded and TRY updated our state.
  // So we return a void success with the new, advanced state.
  return ParseResult<void>::Ok(current_state);
}

ParseError ParseTranslationUnit(const TokenList& tokens, TranslationUnit& tu) {
  ParseState current_state = {0};
  tu.PushScope(u8"global", TU::Scope::Type::Namespace);

  while (current_state.index < tokens.size()) {
    ParseState state_before_decl = current_state;
    current_state = ConsumeTrivia(tokens, current_state);

    if (current_state.index >= tokens.size()) {
      break;
    }
    auto result = ParseTopLevelDeclaration(tokens, tu, current_state);
    if (!result) {
      // A real error occurred. Log it and stop.
      const Token& bad_token = tokens[result.error.token_index];
      // TBD: ... logging ...
      return result.error;
    }

    // If parsing succeeded but no tokens were consumed, it means we have an
    // unhandled token that isn't a top-level declaration.
    if (result.next_state.index == state_before_decl.index) {
      // This prevents infinite loops on unknown tokens.
      return {ParseErrorCode::UnexpectedToken, state_before_decl.index};
    }

    // Commit the new state
    current_state = result.next_state;
  }

  //tu.PopScope();
  return ParseError{ParseErrorCode::Success, current_state.index};
}

}  // namespace aki

#if 0

#endif

#if 0

#include "parser.h"

#include <base/containers/vector.h>
#include <base/logging.h>
#include <base/memory/lazy_instance.h>
#include <base/strings/string_compare.h>

#include "base/compiler.h"
#include "base/strings/string_ref.h"
#include "grammar/syntax.h"
#include "grammar/token.h"
#include "keyword.h"
#include "numeric_format.h"

namespace aki {
static void Utf8ToLowercase(base::StringU8* str) {
  if (str->empty()) {
    return;
  }

  char8_t* data = &(*str)[0];
  size_t length = str->length();
  size_t i = 0;

  while (i < length) {
    unsigned char c = static_cast<unsigned char>(data[i]);
    if (c < 128) {
      // ASCII character
      data[i] = tolower(c);
      i++;
    } else if ((c & 0xE0) == 0xC0 && i + 1 < length) {
      // 2-byte UTF-8 sequence
      if (c == 0xC3 && static_cast<unsigned char>(data[i + 1]) >= 0x80 &&
          static_cast<unsigned char>(data[i + 1]) <= 0x9E) {
        // Latin-1 Supplement uppercase
        data[i + 1] += 0x20;
      }
      i += 2;
    } else if ((c & 0xF0) == 0xE0 && i + 2 < length) {
      // 3-byte UTF-8 sequence
      i += 3;
    } else if ((c & 0xF8) == 0xF0 && i + 3 < length) {
      // 4-byte UTF-8 sequence
      i += 4;
    } else {
      // Invalid UTF-8 sequence, skip
      i++;
    }
  }
}

Parser::Parser(base::Vector<Token> tokens) : tokens_(base::move(tokens)) {}

void Parser::ParseTokens() {
  // Allocate the global space. This is the root of the AST. NOT a namespace.
  objects_.ActivateScope(u8"___AKIGLOBAL__",
                         TranslationUnit::Scope::Type::Namespace);
  // NOTE(Vince): go token by token using an index, so we can peek at indicies
  // that we are not at yet later.
  for (current_index_ = 0; current_index_ < tokens_.size(); current_index_++) {
    const Token& token = tokens_[current_index_];
    if (token.type == TokenType::CharacterSequence) {
      const KeywordType kwd_type = EatKeyword();
      if (kwd_type == KeywordType::Unknown) continue;
      const auto parse_err = ConsumeKeyword(kwd_type, {});
      if (parse_err != ParseError::Success) {
        BASE_LOGI(kTag, "Parse failure: {}", static_cast<i32>(parse_err));
      }
    } else if (token.type == TokenType::Comment) {
      // skip comments
      continue;
    }
  }
}

KeywordType Parser::EatKeyword() {
  const Token& type = tokens_[current_index_];
  KeywordType kwd = MatchKeyword(type.value);
  bool is_match = kwd != KeywordType::Unknown;
  if (is_match) current_index_++;
  // should switch these to array access keyword_array[index] instead of this
  // bs.
  return kwd;
}

ParseError Parser::ConsumeKeyword(
    const KeywordType kwd_type, base::Optional<bool> optional_visibility_info) {
  // test bounds
  if (current_index_ >= tokens_.size()) return ParseError::UnexpectedEnding;

  switch (kwd_type) {
    case KeywordType::Let:
      ParseVariableDecleration(true);
      break;
    case KeywordType::Var:
      ParseVariableDecleration(false);
      break;
    case KeywordType::Private:
      // now we can have a function, class, struct, enum, etc. follow.
      ParseVisiblityDecleration(false);
      break;
    case KeywordType::Public:
      // now we can have a function, class, struct, enum, etc. follow.
      ParseVisiblityDecleration(true);
      break;
    case KeywordType::Namespace: {
      // now a function, class, struct, enum, etc. could follow.
      ParseNamespace();
      break;
    }
    case KeywordType::Func: {
      const ParseResult result = ParseFunctionDecleration();
      if (result.status != ParseError::Success) return result.status;
      objects_.current_scope()->functions.emplace_back(result.maybe_handle);
      break;
    }
    case KeywordType::Enum: {
      const ParseResult result = ParseEnumDecleration();
      if (result.status != ParseError::Success) return result.status;
      objects_.current_scope()->enums.emplace_back(result.maybe_handle);
      break;
    }
    case KeywordType::Class:
    case KeywordType::Struct: {
      BASE_LOGI(
          kTag,
          "Struct/Class are not valid fusion syntax. Use the complex keyword.");
      break;
    }
    case KeywordType::Complex: {
      bool is_pub = !optional_visibility_info.failed()
                        ? optional_visibility_info.value()
                        : false;
      ParseComplexDecleration(is_pub);
      break;
    }
    case KeywordType::Using: {
      ParseAliasDeclerationl();
      break;
    }
    case KeywordType::Extern: {
      ParseExternDecl();
      break;
    }
      // we reserve the legacy word import, so people don't get confused.
    case KeywordType::Include:
    case KeywordType::Import:
      ParseImport();
      break;
    default:
      break;
  }

  return ParseError::Unknown;
}

// returns a ParsedOp handle
ParseResult<ParsedOpRef> Parser::ParseOperand(const bool is_ptr) {
  Token& tok = tokens_[current_index_];
  ParsedOp::Flags flags = ParsedOp::Flags::None;

  if (is_ptr) flags = ParsedOp::Flags::IsPointer;

  switch (tok.type) {
    case TokenType::FloatingNumber:
    case TokenType::Number: {
      const auto expr = objects_.all_ops.Create(ParsedOp::Type::NumericConstant,
                                                flags, tok.value);
      current_index_++;
      BASE_LOGI(kTag, "storing a numeric constant ({})",
                (char*)base::MakeStringCopy(tok.value).c_str());
      return ParseResult(expr.handle);
    }
    case TokenType::BinaryNumber:
    case TokenType::HexNumber:
    case TokenType::OctalNumber: {
      // convert to decimal
      auto conv = ConvertNumericRepresentation(tok.type, tok.value);
      const auto expr =
          objects_.all_ops.Create(ParsedOp::Type::NumericConstant, flags, conv);
      current_index_++;
      BASE_LOGI(kTag, "storing a numeric binary constant");
      return ParseResult(expr.handle);
    }
    case TokenType::Ampersand: {
      DEBUG_TRAP;
      // (WTF?)
      break;
    }
    case TokenType::QuotedString: {
      const auto expr = objects_.all_ops.Create(ParsedOp::Type::QuotedString,
                                                flags, tok.value);
      current_index_++;
      BASE_LOGI(kTag, "storing a string constant (utf8)");
      return ParseResult(expr.handle);
    }
    case TokenType::QuotedStringU16: {
      const auto expr = objects_.all_ops.Create(ParsedOp::Type::QuotedStringU16,
                                                flags, tok.value);
      current_index_++;
      BASE_LOGI(kTag, "storing a string constant (utf16)");
      return ParseResult(expr.handle);
    }
    case TokenType::QuotedStringU32: {
      const auto expr = objects_.all_ops.Create(ParsedOp::Type::QuotedStringU32,
                                                flags, tok.value);
      current_index_++;
      BASE_LOGI(kTag, "storing a string constant (utf32)");
      return ParseResult(expr.handle);
    }
    case TokenType::CharacterSequence: {
      const auto name_ref = ParseCharacterSequence();

      // fn call
      if (CheckForToken(TokenType::LParen)) {
        base::Vector<ParsedExpressionRef> args;
        while (!CheckForToken(TokenType::RParen)) {
          const ParseResult<ParsedExpressionRef> arg =
              ParseExpression(false, false);
          if (arg.status != ParseError::Success)
            return ParseResult<ParsedOpRef>(ParseError::InvalidExpression);
          args.push_back(arg.maybe_handle);

          if (!CheckForToken(TokenType::Comma) &&
              !CheckForToken(TokenType::RParen))
            return ParseResult<ParsedOpRef>(ParseError::MissingComma);
        }
        current_index_++;
        const auto expr = objects_.all_ops.Create(ParsedOp::Type::FunctionCall,
                                                  flags, name_ref);
        return ParseResult(expr.handle);
      }

      BASE_LOGI(kTag, "Storing a variable reference: {}",
                (char*)base::MakeStringCopy(name_ref).c_str());
      // else we treat as variable reference
      const auto expr = objects_.all_ops.Create(
          ParsedOp::Type::VariableReference, flags, name_ref);
      return ParseResult(expr.handle);
    }
    default: {
      // next, see if its a keyword
      const auto kwd = MatchKeyword(tok.value);
      switch (kwd) {
        case KeywordType::True:
        case KeywordType::TRue:
        case KeywordType::FAlse:
        case KeywordType::False: {
          // this is kinda dumb. idk.
          base::StringU8 conv = base::MakeStringCopy(tok.value);
          Utf8ToLowercase(&conv);
          const auto expr =
              objects_.all_ops.Create(ParsedOp::Type::Boolean, flags, conv);
          BASE_LOGI(kTag, "storing a boolean constant");
          current_index_++;
          return ParseResult(expr.handle);
          break;
        }

        default:
          // Handle other cases or do nothing
          break;
      }

      // maybe its something else...
      // const auto name_ref = ParseCharacterSequence();

      break;
    }
  }

  return ParseResult<ParsedOpRef>(ParseError::InvalidOperand);
}

// Examples of expressions:
// Example 1: Simple arithmetic expression
// result = x + y - 3 * z / 2;

// Example 2: Boolean expression
// isValid = (a > 10) && (b < 20) || (c == 0);

// Example 3: Function call expression
// area = computeCircleArea(radius);

// Example 4: Array access expression
// value = myArray[index];

// Example 5: Ternary operator expression
// message = (age >= 18) ? "Adult" : "Minor";
// WARNING: there musnt be recursive calls to parseexpression
ParseResult<ParsedExpressionRef> Parser::ParseExpression(bool has_assignment,
                                                         bool has_ptr) {
  // if this the exp has an assignment, consume it first.
  if (has_assignment && !CheckForToken(TokenType::Equal)) {
    BASE_LOGI(kTag, "Missing assignment");
    return ParseError::MissingAssignment;
  }
  // the sequence of sub-expressions
  base::Vector<ParsedOpRef> stack;

  while (true) {
    Token& tiktok = tokens_[current_index_];
    // try consuming all the operands first
    const ParseResult<ParsedOpRef> res = ParseOperand(has_ptr);
    if (res) {
      // parsedop
      stack.push_back(res.maybe_handle);
    }
    if (tiktok.type == TokenType::Eol) {
      BASE_LOGI(kTag, "ParseExpression(): collected {} operands", stack.size());
      current_index_++;
      break;
    }
    if (CheckForToken(TokenType::Comma)) break;
    if (CheckForToken(TokenType::Semicolon)) break;
  }

  auto stack_size = stack.size();
  auto expr = objects_.all_expressions.Create(base::move(stack));

  if (stack_size > 0) return ParseResult<ParsedExpressionRef>(expr.handle);
  return ParseResult<ParsedExpressionRef>(ParseError::InvalidExpression);
}

ParseError Parser::ParseNamespace() {
  const auto name_ref = ParseCharacterSequence();
  if (name_ref.length() == 0) {
    return ParseError::InvalidName;
  }
  // create tracking object.
  const auto ref = objects_.all_namespaces.Create(name_ref).handle;
  // store as sub namespace of current
  objects_.current_scope()->namespaces.emplace_back(ref);
  // activate
  objects_.ActivateScope(name_ref, TranslationUnit::Scope::Type::Namespace);
  return ParseError::Success;
}

ParseError Parser::ParseVisiblityDecleration(bool is_public) {
  // a core keyword must follow
  auto keyword_index = EatKeyword();
  if (keyword_index == KeywordType::Unknown)
    return ParseError::UnexpectedEnding;

  const auto name_ref = ParseCharacterSequence();
  if (name_ref.length() == 0) {
    return ParseError::InvalidName;
  }

  objects_.ActivateScope(name_ref, TranslationUnit::Scope::Type::Function);
  // parse the body
  Token* body_token = &tokens_[current_index_];
  while (body_token->type != TokenType::RCurly) {
    if (body_token->type == TokenType::CharacterSequence) {
      const KeywordType kwd_type = EatKeyword();
      if (kwd_type != KeywordType::Unknown) {
        ConsumeKeyword(kwd_type, {});
      }
    } else {
      ++current_index_;
    }
    body_token = &tokens_[current_index_];
  }
  objects_.RestoreScope();

  // a link to the past
  const Linkage link = Linkage::Internal;
  const Visibility vis = MakeVisibility(is_public);
  const auto complex_struct =
      objects_.all_complexes.Create(name_ref, link, vis);

  // register the complex to the scope where we came from before parsing this
  // fn, this is done since we entered a child scope already for registering the
  // member scope data: variables, functions, sub classes, etc...
  objects_.current_scope()->complexes.emplace_back(complex_struct.handle);

  return ParseError::Success;
}

ParseResult<ParsedEnumDeclRef> Parser::ParseEnumDecleration() {
  // Check if we are not out of bounds
  if (current_index_ >= tokens_.size()) return ParseError::InvalidName;
  // Parse the enum name
  const auto enum_name = ParseCharacterSequence();
  if (enum_name.empty()) return ParseError::InvalidName;

  base::Vector<base::StringRefU8> enum_values;
  if (!CheckForToken(TokenType::Colon)) {
    // could be extends
    if (EatKeyword() != KeywordType::Extends) return ParseError::MissingColon;

    // it is extends:
    auto extends_name = ParseCharacterSequence();
    auto enum_object = objects_.all_enums.Find(
        [&](ParsedEnumDecl& dd) { return dd.name == extends_name; });
    if (enum_object == nullptr) return ParseError::MissingParent;
    // copy the values, as parent values come before child values
    for (auto& val : enum_object->member_values) enum_values.push_back(val);
  }
  const bool was_extended = enum_values.size() > 0;

  // Parse the enum type
  if (!was_extended) {
    const auto enum_type = ParseType();
    if (enum_type.status != ParseError::Success) return ParseError::InvalidName;
  }

  // Check for opening brace
  if (!CheckForToken(TokenType::LCurly)) return ParseError::MissingBrace;

  // Parse enum values
  while (current_index_ < tokens_.size()) {
    if (CheckForToken(TokenType::RCurly)) break;
    // Skip empty lines
    if (CheckForToken(TokenType::Eol)) continue;
    const auto value_name = ParseCharacterSequence();
    if (value_name.empty()) return ParseError::InvalidName;
    enum_values.push_back(value_name);
    // Check for comma or closing brace
    if (!CheckForToken(TokenType::Comma) && !CheckForToken(TokenType::Eol))
      return ParseError::Missing;
  }
  const auto maybe_handle = objects_.all_enums.Create(
      enum_name, Linkage::Internal, Visibility::Public,
      ParsedEnumDecl::Variant::Integer, base::move(enum_values));
  return ParseResult(maybe_handle.handle);
}

ParseError Parser::ParseParameterDecleration(ParsedParameterDeclRef& handle) {
  // check *again* that we are not out of bounds.
  if (current_index_ >= tokens_.size()) return ParseError::InvalidName;
  const auto parameter_name = ParseCharacterSequence();  // <name>
  if (parameter_name.empty()) return ParseError::InvalidName;
  if (!CheckForToken(TokenType::Colon))  // :
    return ParseError::MissingColon;
  const auto type_name = ParseCharacterSequence();  // <type>
  if (type_name.empty()) return ParseError::InvalidName;
  const auto maybe_handle =
      objects_.all_types.Create(ParsedType::Type::Name, type_name);

  ParsedVariableDecl var(
      parameter_name,                                         // name
      false,                                                  // is_const
      Linkage::Internal,                                      // linkage
      Visibility::Private,                                    // visibility
      maybe_handle.handle,                                    // parsed_type
      (ParsedExpressionRef)TranslationUnit::invalid_handle);  // assignment
  handle = objects_.all_parameters.Create(base::move(var), true).handle;
  return ParseError::Success;
}

// Types of allow fns:
// cast some variables
// func cast_example() {
//   let some_var : i32 = 1000;
//   var as_float : f32 = some_var as f32;
// }
//
// func traditional_function() : i32 {
//  return 1337 * 2;
// }
//
// func oneliner() : return 1337 * 2;
ParseResult<ParsedFunctionDeclRef> Parser::ParseFunctionDecleration() {
  const auto func_name = ParseCharacterSequence();
  if (func_name.empty())  // name
    return ParseError::InvalidName;
  if (!CheckForToken(TokenType::LParen))  // (
    return ParseError::MissingLeftParenthesis;
  // read params
  base::Vector<ParsedParameterDeclRef> param_refs;
  while (true) {
    if (CheckForToken(TokenType::RParen))  // )
      break;

    ParsedParameterDeclRef param_handle =
        (ParsedParameterDeclRef)TranslationUnit::invalid_handle;
    auto result = ParseParameterDecleration(param_handle);
    if (param_handle == TranslationUnit::invalid_handle) return result;
    param_refs.emplace_back(param_handle);
  }
  // function may have a return type ( : <type> )
  // if a semicolon follows, this is an external (or forward declared) function,
  // e.g.: fn foo(a: i32) : i32;
  ParsedTypeRef maybe_return = (ParsedTypeRef)TranslationUnit::invalid_handle;
  if (CheckForToken(TokenType::Colon)) {      // :
    maybe_return = ParseType().maybe_handle;  // <type>
  }

  if (CheckForToken(TokenType::Semicolon)) {
    const auto function_info =
        objects_.all_functions.Create(func_name, param_refs, maybe_return,
                                      Linkage::External, Visibility::Public);
    return ParseResult(function_info.handle);
  }

  // test if the function has a body
  if (!CheckForToken(TokenType::LCurly)) {
    return ParseError::MissingBrace;
  }

  // Activate function context:
  objects_.ActivateScope(func_name, TU::Scope::Type::Function);
  ParseFunctionBody();
  objects_.RestoreScope();

  const auto function_info =
      objects_.all_functions.Create(func_name, param_refs, maybe_return,
                                    Linkage::External, Visibility::Public);
  return ParseResult(function_info.handle);
}

// A statement is a complete instruction that performs some action, so for
// instance a = 5; is a statement. while an expression is a fragment of code
// that produces a value.
// sick set btw : https://youtu.be/rKPBq_j4buQ?si=-a5NZpZ_ehWxdBph&t=4048
ParseResult<ParsedStatementRef> Parser::ParseStatement2() {
  // Must construct a expression object then...

  // example of multiple expressions:
  // a = (b += 2, c += 3, b + c);
  // in a single statement.
  base::Vector<ParsedExpressionRef> expressions;
  base::Vector<ParsedOpRef> all_my_ops_are_coming_for_you;
  while (true) {
    Token* tok = &tokens_[current_index_];
    // todo: handle pointers
    // <name>
    const ParseResult<ParsedOpRef> op_result = ParseOperand(false);
    if (op_result.status != ParseError::Success) return op_result.status;

    all_my_ops_are_coming_for_you.push_back(op_result.maybe_handle);
    // check for assignment <=>
    if (CheckForToken(TokenType::Equal)) {
      const auto expr_result = ParseExpression(false, false);
      if (expr_result.status != ParseError::Success) return expr_result.status;
      expressions.push_back(expr_result.maybe_handle);
    }
    if (CheckForToken(TokenType::Semicolon)) break;
    if (CheckForToken(TokenType::Eol)) break;
    if (CheckForToken(TokenType::Eof)) break;
  }

  // create a "root" expression
  expressions.push_back(
      objects_.all_expressions.Create(base::move(all_my_ops_are_coming_for_you))
          .handle);

  BASE_LOGI(kTag, "ParseStatement2(): collected {} operands",
            all_my_ops_are_coming_for_you.size());
  // For now consider it done:
  // okay there is something we can attach the op to.
  auto parsedexpression = objects_.all_expressions.Create(
      base::move(all_my_ops_are_coming_for_you));
  expressions.push_back(parsedexpression.handle);

  auto result = objects_.all_statements.Create(
      ParsedStatement::Type::Expression, base::move(expressions));
  return result.handle;
}

ParseResult<ParsedStatementRef> Parser::ParseReturnStatement() {
  // consume the keyword
  // current_index_++;
  Token& tok = tokens_[current_index_];

  // a return statement is a statement that causes the function to return a
  // value but we allow expressions to be returned, so we have to parse the
  // expression
  base::Vector<ParsedExpressionRef> return_expressions;

  auto expr = ParseExpression(false, false);
  if (expr.status != ParseError::Success) return expr.status;

  // TODOTODOTODO: handle multiple return values
  // these would work like this:
  // similar to return value optimization, we would pass them as ptr out params.
  return_expressions.push_back(expr.maybe_handle);

  const auto ret_info = objects_.all_statements.Create(
      ParsedStatement::Type::Return, base::move(return_expressions));
  return ret_info.handle;
}

ParseError Parser::ParseFunctionBody() {
  aki::TU::Scope& body = *objects_.current_scope();
  // parse the body
  while (true) {
    auto top_val = current_index_;
    const Token& tok = tokens_[current_index_];
    auto val = tok.value;
    switch (tok.type) {
      case TokenType::RCurly:
      case TokenType::Eof:  // end of file
        // we are done
        return ParseError::Success;
      case TokenType::Eol:
      case TokenType::Semicolon:
      case TokenType::Comment: {
        current_index_++;
        break;
      }
      case TokenType::CharacterSequence: {
        const KeywordType kwd_type = EatKeyword();
        switch (kwd_type) {
          case KeywordType::Let:
          case KeywordType::Var: {
            ParseVariableDecleration(kwd_type == KeywordType::Let);
            break;
          }
          case KeywordType::Return: {
            const auto res = ParseReturnStatement();
            if (res.status != ParseError::Success) return res.status;
            body.AddObject(TU::ObjectType::Statement, res.maybe_handle);
            break;
          }
          default: {
            // This might be a function call or an assignment
            BASE_LOGI(kTag, "VALUEEEEEEEEEEEEE: {}",
                      (char*)base::MakeStringCopy(val).c_str());
            const auto expr_result = ParseStatement2();
            if (expr_result.status != ParseError::Success) break;
            body.AddObject(TU::ObjectType::Statement, expr_result.maybe_handle);
            BASE_LOGI(kTag, "PARSE SUCCECSS YAY!!!!");
            break;
          }
        }
        break;
      }
      default:
        return ParseError::UnexpectedToken;
    }
  }
  return ParseError::Success;
}

ParseError Parser::ParseComplexDecleration(bool is_public) {
  const auto name_ref = ParseCharacterSequence();
  if (name_ref.length() == 0) return ParseError::InvalidName;

  // generic
  // <
  if (CheckForToken(TokenType::LessThan)) {
    // T
    const auto generic_name_ref = ParseCharacterSequence();
    if (generic_name_ref.length() == 0) return ParseError::InvalidName;
    // Make sure its T, for now
    if (generic_name_ref.data()[0] != u8'T') return ParseError::InvalidName;
    // =
    if (!CheckForToken(TokenType::Equal)) return ParseError::UnexpectedEnding;
    // <type>
    const ParseResult type = ParseType();
    if (!CheckForToken(TokenType::GreaterThan))
      return ParseError::UnexpectedEnding;
  }

  if (!CheckForToken(TokenType::LCurly)) {
    return ParseError::MissingBrace;
  }

  // parse the body
  Token* body_token = &tokens_[current_index_];
  while (body_token->type != TokenType::RCurly) {
    if (body_token->type == TokenType::CharacterSequence) {
      const KeywordType kwd_type = EatKeyword();
      if (kwd_type != KeywordType::Unknown) {
        ConsumeKeyword(kwd_type, {});
      }
    } else {
      ++current_index_;
    }
    body_token = &tokens_[current_index_];
  }

  // a link to the past
  const Linkage link = Linkage::Internal;
  const Visibility vis = MakeVisibility(is_public);

  // not worth deleting...
  const auto complex_struct =
      objects_.all_complexes.Create(name_ref, link, vis);

  objects_.current_scope()->complexes.emplace_back(complex_struct.handle);

  return ParseError::Success;
}

ParseError Parser::ParseAliasDeclerationl() { return ParseError(); }

// https://github.com/SerenityOS/jakt/blob/30563dbef0f77aad663aa14cf76f4fb1727f0947/src/parser.rs#L2037
// let binary_number_2 : i32 = 0B0101;
// let utf8_str :        c8& = "Hello, World!";
ParseError Parser::ParseVariableDecleration(
    bool is_const /*let for const, or var for mutable*/) {
  // let/var nmame : aaa = b;
  const auto name_ref = ParseCharacterSequence();  // nmame
  if (name_ref.empty()) return ParseError::InvalidName;

  BASE_LOGI(kTag, "Parse attempt: {}",
            (char*)base::MakeStringCopy(name_ref).c_str());

  // type is provided.
  if (CheckForToken(TokenType::Colon)) {  // :
    const bool is_array = CheckForToken(TokenType::LSquare);
    BASE_LOGI(kTag, "Array: {}", is_array ? "true" : "false");

    ParseResult<ParsedTypeRef> parsed_type;
    if (!is_array)
      parsed_type = ParseType();  // aaa
    else if (is_array) {
      // [i32, ...] or [i32, 10] are allowed
      parsed_type = ParseType();
      if (parsed_type.status != ParseError::Success) return parsed_type.status;
      if (!CheckForToken(TokenType::Comma))
        return ParseError::UnexpectedEnding;                    // ,
      bool is_unbounded = CheckForToken(TokenType::DotDotDot);  // ...
      BASE_LOGI(kTag, "Array is Unbounded: {}",
                is_unbounded ? "true" : "false");
      if (is_unbounded) {  // [i32, ...]
        if (!CheckForToken(TokenType::RSquare)) {
          BASE_LOGW(kTag, "Expected closing bracket for array decleration");
          return ParseError::UnexpectedEnding;  // WARNING CAUSES INFINITE LOOP
        }
      } else {  // exact array size: [i32, 10]
        // TBD
      }
    }

    // a ptr decleration might follow, such as
    // let utf8_ptr :        c8& = "Hello, World!";
    const bool is_ptr = CheckForToken(TokenType::Ampersand);

    // read out the assignment (=something)
    const ParseResult<ParsedExpressionRef> expr_result =
        ParseExpression(true, is_ptr);

    auto obj =
        expr_result.status == ParseError::Success
            ? objects_.all_variables.Create(
                  name_ref, is_const, Linkage::External, Visibility::Private,
                  parsed_type.maybe_handle, expr_result.maybe_handle)
            : objects_.all_variables.Create(
                  name_ref, is_const, Linkage::External, Visibility::Private,
                  parsed_type.maybe_handle,
                  (ParsedExpressionRef)TU::invalid_handle);

    objects_.current_scope()->AddObject(TU::ObjectType::Variable, obj.handle);
    return ParseError::Success;
  }
  // immedeate assignment (auto type deduction)
  // no point in parsing...
  else if (Peek().type == TokenType::Equal) {
    auto obj = objects_.all_variables.Create(
        name_ref, is_const, Linkage::External, Visibility::Private,
        (ParsedTypeRef)TU::invalid_handle,
        (ParsedExpressionRef)TU::invalid_handle);

    objects_.current_scope()->AddObject(TU::ObjectType::Variable, obj.handle);
    BASE_LOGI(kTag, "Parsing auto type..");

    return ParseError::Success;
  } else {
    BASE_LOGI(kTag, "Peek type: {} unknown.. owie.. ",
              static_cast<i32>(Peek().type));
    return ParseError::InvalidTypeDecleration;
  }
}

ParseError Parser::ParseImport() {
  const auto name_ref = ParseString();
  if (name_ref.empty()) return ParseError::InvalidName;
  // store the token.
  current_index_++;

  auto ref = objects_.all_imports.Create(name_ref).handle;
  objects_.current_scope()->imports.emplace_back(ref);

  return ParseError::Success;
}

ParseError Parser::ParseExternDecl() {
  const auto name_ref = ParseString();
  if (name_ref.empty()) return ParseError::InvalidName;
  // if (name_ref == u8"c")
  //   return ParseError::Success;
  return ParseError::Unknown;
}

base::StringRefU8 Parser::ParseCharacterSequence() {
  const Token& token = tokens_[current_index_];
  current_index_++;
  return token.type == TokenType::CharacterSequence ? token.value : u8"";
}

base::StringRefU8 Parser::ParseString() {
  const Token& tok = tokens_[current_index_];
  current_index_++;
  return tok.type == TokenType::QuotedString ? tok.value : u8"";
}

bool Parser::CheckForToken(TokenType type) {
  if (current_index_ >= tokens_.size()) {
    return false;
  }

  const Token& tok = tokens_[current_index_];
  if (tok.type != type) {
    return false;
  }

  current_index_++;
  return true;
}

ParseResult<ParsedTypeRef> Parser::ParseType() {
  Token* tok = &tokens_[current_index_];
  current_index_++;

  ParsedType::Type type_type{ParsedType::Type::Empty};
  u32 extra_data = 0;
  switch (tok->type) {
    // TBD: might be an array:
    case TokenType::LSquare: {
      tok = &tokens_[current_index_];
      current_index_++;
      type_type = ParsedType::Type::Array;
      // auto &type
      break;
    }
    case TokenType::CharacterSequence: {
      const KeywordType type = MatchKeyword(tok->value);
      // to speed everything up, before doing type resolution,
      // we see if this is a known inbuilt
      // example: let number : i32 = 0;
      //                       ^^^
      if (IsCoreNumericType(type)) {
        type_type = ParsedType::Type::Name;
      }
      break;
    }
    default:
      type_type = ParsedType::Type::Empty;
      break;
  }

  // lets store it
  const auto new_type =
      objects_.all_types.Create(type_type, (*tok).value, extra_data);

  return ParseResult(new_type.handle);
}
}  // namespace aki

#endif