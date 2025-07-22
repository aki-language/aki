// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include "syntax.h"

namespace aki {

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
}  // namespace aki
