// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include "ast.h"
#include "type.h"

namespace aki {
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

}