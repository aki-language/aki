#pragma once

#include <base/strings/xstring.h>

#include "base/strings/string_ref.h"

namespace insane {
struct TranslationUnit;

class CodeGen {
 public:
  virtual ~CodeGen() = default;

  // https://github.com/SerenityOS/jakt/blob/30563dbef0f77aad663aa14cf76f4fb1727f0947/src/compiler.rs#LL241C21-L241C28
  // https://github.com/SerenityOS/jakt/blob/30563dbef0f77aad663aa14cf76f4fb1727f0947/src/codegen.rs#L185
  // virtual void Emit() = 0;

  virtual void GenerateCode(TranslationUnit&) = 0;

  virtual const base::StringRefU8 GetTextBuffer() const = 0;
};
}  // namespace insane
