// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include "ast.h"

namespace aki {
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
}