// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include "ast.h"

namespace aki {
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
}