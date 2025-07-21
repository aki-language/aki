// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023
#pragma once

#include <base/containers/vector.h>
#include <base/memory/unique_pointer.h>

#include "../utils/fifo_stack.h"
#include "base/compiler.h"

#include <utils/object_pool.h>
#include <grammar/syntax.h>

namespace aki {
#define AST_POOL(class_name, name) \
  ObjectPool<class_name, class_name##Ref, obj_handle> all_##name

// This class stores all data about a translation unit.
// It is the root of the AST.
struct TranslationUnit {
  static constexpr auto invalid_handle = obj_handle(-1);

  enum class ObjectType {
    Import,
    Namespace,
    Variable,
    Parameter,
    Function,
    Enum,
    Complex,
    Statement,
    Type,
  };
  // all data is kept here, in object pools.
  AST_POOL(ParsedImport, imports);
  AST_POOL(ParsedNamespace, namespaces);
  AST_POOL(ParsedVariableDecl, variables);
  AST_POOL(ParsedParameterDecl, parameters);
  AST_POOL(ParsedFunctionDecl, functions);
  AST_POOL(ParsedEnumDecl, enums);
  AST_POOL(ParsedComplexDecl, complexes);
  AST_POOL(ParsedExpression, expressions);
  AST_POOL(ParsedStatement, statements);
  AST_POOL(ParsedType, types);
  AST_POOL(ParsedOp, ops);

  // scopes can own a set of objects, these can be either: namespaces,
  // functions, or complexes. SCOPES only exist here, as a high level concept.
  // They are not part of the AST.
  struct Scope {
    enum class Type { Namespace, Function, Complex };

    explicit Scope(const base::StringRefU8 name, const Type t) : name(name), type(t) {}

    const base::StringRefU8 name;
    const Type type;

    base::Vector<ParsedNamespaceRef> namespaces;
    base::Vector<ParsedImportRef> imports;
    base::Vector<ParsedVariableDeclRef> global_variables;
    base::Vector<ParsedFunctionDeclRef> functions;
    base::Vector<ParsedEnumDeclRef> enums;
    base::Vector<ParsedComplexDeclRef> complexes;
    base::Vector<ParsedStatementRef> statements;

    bool all_empty() const {
      return namespaces.empty() && functions.empty() && complexes.empty() &&
             global_variables.empty() && imports.empty() && statements.empty();
    }

    struct BigHandle {
      const ObjectType type;
      const obj_handle handle;
    };
    base::Vector<BigHandle> tree;

    template <typename T>
    void AddObject(const ObjectType obj_type, const T ref) {
      if constexpr (std::is_same_v<T, ParsedImportRef>) {
        imports.push_back(ref);
      } else if constexpr (std::is_same_v<T, ParsedNamespaceRef>) {
        namespaces.push_back(ref);
      } else if constexpr (std::is_same_v<T, ParsedVariableDeclRef>) {
        global_variables.push_back(ref);
      } else if constexpr (std::is_same_v<T, ParsedFunctionDeclRef>) {
        functions.push_back(ref);
      } else if constexpr (std::is_same_v<T, ParsedEnumDeclRef>) {
        enums.push_back(ref);
      } else if constexpr (std::is_same_v<T, ParsedComplexDeclRef>) {
        complexes.push_back(ref);
      } else if constexpr (std::is_same_v<T, ParsedStatementRef>) {
        statements.push_back(ref);
      } else {
        DEBUG_TRAP;  // WTF are you doing?!?
      }

      tree.push_back({obj_type, static_cast<obj_handle>(ref.value)});
    }
    // base::Vector<obj_handle> expressions;
  };

 private:
  static constexpr char kLogTag[] = "translationunit";

  Scope* current_scope_ = nullptr;
  base::Vector<base::UniquePointer<Scope>> scopes_storage;
  std::stack<Scope*> scope_stack_;

 public:
  Scope* current_scope() {
    BASE_BUGCHECK(current_scope_, "No scope exists to attach data to.");
    return current_scope_;
  }

  Scope* global_scope() {
    BASE_BUGCHECK(!scopes_storage.empty(), "No global scope exists.");
    return scopes_storage.front().Get_UseOnlyIfYouKnowWhatYouareDoing();
  }

  Scope* FindAttachedScope(const base::StringRefU8 name) const {
    for (auto& scope : scopes_storage) {
      if (scope.Get_UseOnlyIfYouKnowWhatYouareDoing()->name == name)
        return scope.Get_UseOnlyIfYouKnowWhatYouareDoing();
    }
    return nullptr;
  }

  auto& all_scopes() { return scopes_storage; }

  inline Scope& PushScope(const base::StringRefU8 name, Scope::Type type) {
    Scope* ptr = nullptr;
    // Do we have a scope with the same name?
    ptr = FindAttachedScope(name);

    if (!ptr) {
      // adding to the storage will invalidate the reference
      // so we store em in a unique pointer.
      base::UniquePointer<Scope>& object =
          scopes_storage.emplace_back(base::MakeUnique<Scope>(name, type));

      ptr = object.Get_UseOnlyIfYouKnowWhatYouareDoing();
    }

    auto tmp_str = name.to_string();
    BASE_LOGI(kLogTag, ">>> Push scope: {}", (const char*)tmp_str.c_str());

    scope_stack_.push(ptr);
    current_scope_ = ptr;
    return *ptr;
  }

  inline void PopScope() {
    BASE_LOGI(kLogTag, "<<< Pop scope: {}",
              (const char*)current_scope_->name.to_string().c_str());
    scope_stack_.pop();
    if (!scope_stack_.empty()) {
      current_scope_ = scope_stack_.top();
    } else {
      current_scope_ = nullptr;  // No more scopes left
    }
  }
};

using TU = TranslationUnit;
}  // namespace aki
