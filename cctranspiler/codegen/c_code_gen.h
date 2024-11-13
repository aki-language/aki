// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023

#include "code_gen.h"
#include "text_printer.h"

#include <base/containers/vector.h>
#include <base/strings/xstring.h>

#include "../grammar/translation_unit.h"

namespace insane {
// NOTE(Vince): This is a very simple code emitter that only supports emitting C
// code, which is awesome for portability during the early stages of this
// project.
class CCodeGen final : public CodeGen {
 public:
  CCodeGen();

  // Inherited via CodeGen
  void GenerateCode(TranslationUnit&) override;

  void EmitScope(const TranslationUnit::Scope&, const TranslationUnit&);

 private:
  void CreateEntrypoint();

  void EmitImport(const ParsedImport&);
  void EmitVariable(const TranslationUnit&,
                    const ParsedVariableDecl&,
                    const base::StringRefU8,
                    const bool needs_mangeling = true);

  // functions
  void EmitFunctionBlock(const TranslationUnit&,
                         const TranslationUnit::Scope&,
                         const ParsedFunctionDecl&,
                         const base::StringRefU8);
  void EmitFunction(const ParsedFunctionDecl&, const base::StringRefU8);

  void EmitStruct(const ParsedComplexDecl&, const base::StringRefU8);
  void EmitEnum(const ParsedEnumDecl&, const base::StringRefU8);

 private:
  base::StringU8 buffer_;
  base::StringU8 entry_symbol_;
  TextPrinter printer_;
  TranslationUnit* translation_unit_{nullptr};
};

}  // namespace insane
