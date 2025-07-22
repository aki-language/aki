// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include "ast.h"

namespace aki {
/*
Statement
|
+-- Expression
|   |
|   +-- Operand
|   |
|   +-- Operator
|   |
|   +-- Operand
|
+-- Semicolon

Example: x = a + b;
         | | | | |
         | | | | +-- Semicolon (ends the statement)
         | | | +---- Operand (b)
         | | +------ Operator (+)
         | +-------- Operand (a)
         +---------- Assignment operator (=)

The entire line is a statement.
'x = a + b' is an expression within the statement.
'a + b' is a sub-expression within the assignment expression.
'x', 'a', and 'b' are operands.
*/

// This struct represents both operators and operands.
struct ParsedOp {
  enum class Type : u16 {
    // Operands
    Boolean,
    NumericConstant,
    QuotedString,
    QuotedStringU16,
    QuotedStringU32,
    CharacterLiteral,
    ByteLiteral,
    Array,
    Dictionary,
    Set,
    IndexedExpression,
    UnaryOp,
    BinaryOp,
    Var,
    NamespacedVar,
    Tuple,
    Range,
    Match,
    IndexedTuple,
    IndexedStruct,
    FunctionCall,
    MethodCall,
    VariableReference,
    ForcedUnwrap,
    OptionalNone,
    OptionalSome,
    Operator,
    // Operators
    Assignment,
    Arithmetic,
    Comparison,
    Logical,
    Bitwise,
    Unary,
    // Junk
    Garbage,
  };

  enum class Flags : u16 {
    None = 0,
    IsOperand = 1 << 0,
    IsPointer = 1 << 1,
    IsConstant = 1 << 2,
    IsMutable = 1 << 3,
    IsTemporary = 1 << 4,
    IsVolatile = 1 << 5,
    IsArray = 1 << 6,
    IsFunction = 1 << 7,
    IsMethod = 1 << 8,
    IsLValue = 1 << 9,
    IsRValue = 1 << 10,
    IsStatic = 1 << 11,
  };

  Type type;
  Flags flags;
  base::StringRefU8 data;

  ParsedOp(Type t, Flags f, const base::StringRefU8& data)
      : type(t), flags(f), data(data) {}
  ParsedOp(ParsedOp&& other) noexcept
      : type(other.type), flags(other.flags), data(std::move(other.data)) {
    // No need for any additional steps since data is of const type
  }
  ParsedOp& operator=(ParsedOp&& other) noexcept {
    if (this != &other) {
      type = other.type;
      flags = other.flags;
      data = base::move(other.data);
    }
    return *this;
  }

  BASE_NOCOPY(ParsedOp);
};
AST_HANDLE(ParsedOp)
BASE_IMPL_ENUM_BIT_TRAITS(ParsedOp::Flags, u16)

// Expression
struct ParsedExpression {
  // references to related sub expressions...
  // (in order of evaluation, left to right)
  base::Vector<ParsedOpRef> stack;

  ParsedExpression(base::Vector<ParsedOpRef>&& stack) : stack(base::move(stack)) {}
  ParsedExpression(ParsedExpression&& other) noexcept : stack(base::move(other.stack)) {}
  ParsedExpression& operator=(ParsedExpression&& other) noexcept {
    if (this != &other) {
      stack = base::move(other.stack);
    }
    return *this;
  }

  BASE_NOCOPY(ParsedExpression);
};
AST_HANDLE(ParsedExpression)

struct ParsedStatement {
  enum class Type {
    Expression,
    Defer,
    UnsafeBlock,
    VarDecl,
    If,
    Block,
    Loop,
    While,
    For,
    Break,
    Continue,
    Return,
    Yield,
    Throw,
    Try,
    InlineCpp,
    Garbage
  };

  Type type;
  base::Vector<ParsedExpressionRef> expr_stack;

  ParsedStatement(const Type t, base::Vector<ParsedExpressionRef>&& h)
      : type(t), expr_stack(base::move(h)) {}

  // Move constructor
  ParsedStatement(ParsedStatement&& other) noexcept
      : type(other.type), expr_stack(base::move(other.expr_stack)) {}
};
AST_HANDLE(ParsedStatement)
}