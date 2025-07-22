// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <any>
#include <optional>
#include <variant>
#include <vector>

#include "base/containers/vector.h"
#include "base/logging.h"
#include "base/memory/move.h"
#include "base/optional.h"
#include "token.h"

// NOTE(Vince): Keep the data types as simple as possible, with great granularity.
// Reference other types with obj_handle (and thus AST_HANDLE) references. This will
// ensure that our structs have a greater chance to fit into the cpu cache lines.
namespace aki {
using obj_handle = u64;
inline constexpr obj_handle kInvalidHandleValue = static_cast<obj_handle>(-1);

// i love c++
// we still must do this though, in order to enforce a type safe and responsible handle
// system.
#define AST_HANDLE(HandleName)                                                 \
  struct HandleName##Ref {                                                     \
    obj_handle value;                                                          \
    using subtype = obj_handle;                                                \
                                                                               \
    constexpr HandleName##Ref() : value(kInvalidHandleValue) {}                \
    explicit constexpr HandleName##Ref(obj_handle v) : value(v) {}             \
                                                                               \
    [[nodiscard]] bool IsValid() const {                                       \
      return value != kInvalidHandleValue;                                     \
    }                                                                          \
    friend bool operator==(const HandleName##Ref a, const HandleName##Ref b) { \
      return a.value == b.value;                                               \
    }                                                                          \
    friend bool operator==(const HandleName##Ref a, obj_handle b) {            \
      return a.value == b;                                                     \
    }                                                                          \
    friend bool operator==(obj_handle a, const HandleName##Ref& b) {           \
      return a == b.value;                                                     \
    }                                                                          \
    friend bool operator!=(const HandleName##Ref a, const HandleName##Ref b) { \
      return a.value != b.value;                                               \
    }                                                                          \
  };

struct ScalarOrRef {
  union {
    base::StringRefU8 name;
    u64 num;
  } data;
};

enum class Visibility : u8 {
  Public,
  Private,
};

inline Visibility MakeVisibility(const bool is_public) {
  return is_public ? Visibility::Public : Visibility::Private;
}

enum class Linkage : u8 {
  Internal,
  External,
};

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

struct ParsedCall {
  const base::StringRefU8 name;
  base::Vector<base::String> namespaces;
  // todo: args...
};
AST_HANDLE(ParsedCall)

// Example: import "file_name";
struct ParsedImport {
  const base::StringRefU8 target;
};
AST_HANDLE(ParsedImport)

struct ParsedNamespace {
  const base::StringRefU8 name;
};
AST_HANDLE(ParsedNamespace)

AST_HANDLE(ParsedType)
struct ParsedType {
  AST_HANDLE(NamespacedName)
  AST_HANDLE(GenericType)
  AST_HANDLE(Tuple)
  AST_HANDLE(Array)
  AST_HANDLE(Dictionary)
  AST_HANDLE(Set)
  AST_HANDLE(Optional)
  AST_HANDLE(Pointer)

  struct NamespacedName {
    base::Vector<base::StringRefU8> segments;  // e.g., ["std", "io", "File"]
  };
  struct GenericType {
    ParsedTypeRef base_type;  // Handle to the base type, e.g., `Vec`
    base::Vector<ParsedTypeRef>
        generic_args;  // Handles to the types in <...>, e.g., `i32`
  };
  struct Tuple {
    base::Vector<ParsedTypeRef> element_types;
  };
  struct Array {
    ParsedTypeRef element_type;     // the type of elements in the array.
    ParsedExpressionRef size_expr;  // invalid_handle if unbounded.
  };
  struct Dictionary {
    ParsedTypeRef key_type;
    ParsedTypeRef value_type;
  };
  struct Set {
    ParsedTypeRef element_type;
  };
  struct Optional {
    ParsedTypeRef underlying_type;  // The type being wrapped by '?'
  };
  // handles both RawPtr and WeakPtr
  struct Pointer {
    ParsedTypeRef pointee_type;  // the type being pointed to.
  };

  // raw text of the type
  base::StringRefU8 data;

  enum class Type {
    Name,
    NamespacedName,
    GenericType,
    GenericResolvedType,
    Tuple,
    Array,
    Dictionary,
    Set,
    Optional,
    RawPtr,
    WeakPtr,
    Empty,
  } type;

  // the actual type data
  union {
    ParsedNamespaceRef namespaced_name;   // e.g., std::io::File
    GenericTypeRef generic_type;          // e.g., Vec<i32>
    ParsedTypeRef generic_resolved_type;  // e.g., Vec<i32> resolved to a concrete type
    TupleRef tuple;                       // e.g., (i32, f32)
    ArrayRef array;                       // e.g., [i32; 10] or [i32, ...]
    DictionaryRef dictionary;             // e.g., {i32: f32}
    SetRef set;                           // e.g., {i32, ...}
    OptionalRef optional;                 // e.g., i32?
    PointerRef raw_ptr;                   // e.g., *const i32 or *mut i32
    PointerRef weak_ptr;                  // e.g., *const i32 or *mut i32
    obj_handle generic_handle;            // punning to a ParsedTypeRef
  } detail{
      .generic_handle = kInvalidHandleValue  // Initialize to invalid handle
  };

  // default empty constructor
  ParsedType() : type(Type::Empty), data(base::StringRefU8::null_ref()) {}

  template <typename TRef>
  ParsedType(const Type t, const base::StringRefU8 data, TRef detail_handle = 0)
      : type(t), data(data) {
    detail.generic_handle = detail_handle.value;
  }
  // Move constructor
  ParsedType(ParsedType&& other) noexcept : type(other.type), data(other.data) {
    detail = other.detail;  // Move the union data
  }
  // Copy constructor (implicitly deleted due to the presence of a user-defined
  // move constructor)
  ParsedType(const ParsedType& other) : type(other.type), data(other.data) {
    detail = other.detail;  // Copy the union data
  }
};

// i guess we have to seperate variable decl from variable use...

struct ParsedEnumDecl {
  const base::StringRefU8 name;
  const Linkage linkage;
  const Visibility visibility;
  enum class Variant {
    String,
    Integer,
  } variant;

  base::Vector<base::StringRefU8> member_values;

  explicit ParsedEnumDecl(const base::StringRefU8 name,
                          const Linkage linkage,
                          const Visibility visibility,
                          const Variant variant,
                          base::Vector<base::StringRefU8>&& refu8)
      : name(name),
        linkage(linkage),
        visibility(visibility),
        variant(variant),
        member_values(base::move(refu8)) {}

  // Move constructor
};
AST_HANDLE(ParsedEnumDecl)

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

struct ParsedVariableDecl {
  const base::StringRefU8 name;
  // pack to 4word boundary
  const bool is_const;
  const Linkage linkage;
  const Visibility visibility;
  // type info
  ParsedTypeRef type;
  ParsedExpressionRef optional_assignment;

  explicit ParsedVariableDecl(const base::StringRefU8 name,
                              const bool is_const,
                              const Linkage linkage,
                              const Visibility visibility,
                              const ParsedTypeRef parsed_type,
                              const ParsedExpressionRef assignment)
      : name(name),
        is_const(is_const),
        linkage(linkage),
        visibility(visibility),
        type(parsed_type),
        optional_assignment(assignment) {}
#if 1
  ParsedVariableDecl(ParsedVariableDecl&& other) noexcept
      : name(base::move(other.name)),
        is_const(other.is_const),
        linkage(other.linkage),
        visibility(other.visibility),
        type(other.type),
        optional_assignment(other.optional_assignment) {}
#endif

  BASE_NOCOPY(ParsedVariableDecl);
};
AST_HANDLE(ParsedVariableDecl)

struct ParsedParameterDecl {
  ParsedVariableDecl variable;
  bool requires_label;

  explicit ParsedParameterDecl(ParsedVariableDecl&& variable, bool requires_label)
      : variable(base::move(variable)), requires_label(requires_label) {}
};
AST_HANDLE(ParsedParameterDecl)

// Example: <visibility> func name (param1, param2, ...) : <return type> {}
struct ParsedFunctionDecl {
  const base::StringRefU8 name;
  const Linkage linkage;
  const Visibility visibility;
  const ParsedTypeRef return_type;
  base::Vector<ParsedParameterDeclRef> param_refs;

  explicit ParsedFunctionDecl(const base::StringRefU8 name,
                              base::Vector<ParsedParameterDeclRef>& params,
                              ParsedTypeRef return_type,
                              Linkage linkage,
                              Visibility visibility)
      : name(name),
        param_refs(base::move(params)),
        return_type(return_type),
        linkage(linkage),
        visibility(visibility) {}

  // Move constructor
  ParsedFunctionDecl(ParsedFunctionDecl&& other) noexcept
      : name(std::move(other.name)),
        param_refs(std::move(other.param_refs)),
        return_type(other.return_type),
        linkage(other.linkage),
        visibility(other.visibility) {}
  BASE_NOCOPY(ParsedFunctionDecl);
};
AST_HANDLE(ParsedFunctionDecl)

struct ParsedComplexDecl {
  const base::StringRefU8 name;
  const Linkage linkage;
  const Visibility visibility;
  base::Vector<ParsedType> generic_types;
  base::Vector<ParsedFunctionDecl> member_functions;
  // base::Vector<ParsedVariableDecl> member_variables;

  explicit ParsedComplexDecl(const base::StringRefU8 name,
                             const Linkage linkage,
                             const Visibility visibility)
      : name(name), linkage(linkage), visibility(visibility) {}

  // Move constructor
  ParsedComplexDecl(ParsedComplexDecl&& other) noexcept
      : name(std::move(other.name)),
        linkage(other.linkage),
        visibility(other.visibility),
        generic_types(std::move(other.generic_types)),
        member_functions(std::move(other.member_functions)) {}

  BASE_NOCOPY(ParsedComplexDecl);
};
AST_HANDLE(ParsedComplexDecl)
}  // namespace aki
