// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include <base/strings/string_ref.h>
#include <base/memory/move.h>
#include <base/containers/vector.h>

// NOTE(Vince): Keep the AST data types as simple as possible, with great granularity.
// Reference other types with obj_handle (and thus AST_HANDLE) references. This will
// ensure that our structs have a greater chance to fit into the cpu cache lines.
namespace aki {
using obj_handle = u64;
inline constexpr obj_handle kInvalidHandleValue = static_cast<obj_handle>(-1);

// NOTE(Vince): i *really* love c++
// we still must do this though, in order to enforce a type safe and responsible handle
// system, where you have to *think* about what you are doing
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

// Some common primitive types:
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

// HERE FOR NOW, MOVE LATER
// Example: import "file_name";
struct ParsedImport {
  const base::StringRefU8 target;
};
AST_HANDLE(ParsedImport)

struct ParsedNamespace {
  const base::StringRefU8 name;
};
AST_HANDLE(ParsedNamespace)

}  // namespace aki
