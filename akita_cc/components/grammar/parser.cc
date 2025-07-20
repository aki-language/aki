// Copyright (C) The Fusion Authors/Vincent Hengel 2023

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

constexpr char kTag[] = "langparser";

// NOTE(Vince): this class exists on a per translation unit basis. therefore it
// has no knowledge of the other translation units (functions, variables etc
// defined within other aki source files).
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