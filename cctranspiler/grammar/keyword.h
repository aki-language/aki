// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <base/strings/string_ref.h>

// IDEA(Vince): a by keyword?
namespace aki {
enum class KeywordType {
  As,  // type as other_type;

  Is,  // type is other_type;

  IsA,  // type is_a other_type;

  Let,  // let name = value;
  Var,  // var name = value;
  Raw,  // raw"string that can extend multiple lines";

  Enum,     // enum name { a, b, c };
  Class,    // class name {...};
  Struct,   // struct name {...}; everything public
  Complex,  // complex name {...}; accessibility depending on prepended keyword,
            // by default private. succeeds struct, class in the fusion
            // language. they are only kept as keywords for error collecting
            // reasons.
  Extends,  // complex A extends B {}
  Using,    // using <type_name> = type<T...>; // alias type name to template
            // combination another name idea could be adopt
  Import,   // import "source.fu"
  Include,  // same as Import, supported for compatability reasons.

  Extern,  // extern "C" { int my_c_code = 10; } or extern "C++" {
           // template<typename T> void my_templated_mess() {...} }

  Func,     // func name(params...) {...}
  Return,   // return 0;
  Public,   // public func name() {}
  Private,  // only works for access control in classes
  Switch,   // switch (a) { case 0: {} }

  Match,  // match (condition a , condition b or condition c c) { case 0: {} }

  If,  // if (condition) {} else if (condition) {} else {}

  For,  // IDEA: for (int i .. 10) {} one by one to ten, or for (int i .. 10 ..
        // 2) {} one by one to ten, but with a step of two

  While,     // while (condition) {}
  Break,     // break;
  Continue,  // continue;
  // coupling
  Namespace,  // namespace name {}
  // boolean
  Bool,  // let a : bool = true;

  // operands:
  True,   // let a : bool = true;
  False,  // let a : bool = false;
  TRue,   // let a : bool = TRUE;
  FAlse,  // let a : bool = FALSE;

  // numeric sizes
  I8,   // let a : i8 = 0;
  U8,   // let a : u8 = 0;
  I16,  //
  U16,
  I32,
  U32,
  I64,
  U64,
  I128,
  U128,  //
  // floats
  F32,
  F64,
  F128,
  // characters
  C8,
  C16,
  C32,

  Unknown,
  COUNT
};

const char8_t* KeywordToName(KeywordType type) noexcept;
KeywordType MatchKeyword(const base::StringRefU8) noexcept;

// inbuilt numeric type
inline bool IsCoreNumericType(const KeywordType type) noexcept {
  switch (type) {
    case KeywordType::I8:
    case KeywordType::U8:
    case KeywordType::I16:
    case KeywordType::U16:
    case KeywordType::I32:
    case KeywordType::U32:
    case KeywordType::I64:
    case KeywordType::U64:
    case KeywordType::I128:
    case KeywordType::U128:
    case KeywordType::F32:
    case KeywordType::F64:
    case KeywordType::F128:
    case KeywordType::C8:
    case KeywordType::C16:
    case KeywordType::C32:
      return true;
    default:
      return false;
  }
}
}  // namespace insane
