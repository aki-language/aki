// Copyright (C) Vincent Hengel 2025
#pragma once

#include <base/arch.h>
#include "variable.h"
#include "type.h"

namespace aki {
struct ParsedCall {
  const base::StringRefU8 name;
  base::Vector<base::String> namespaces;
  // todo: args...
};
AST_HANDLE(ParsedCall)

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

}  // namespace aki