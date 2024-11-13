// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "code_gen_factory.h"

#include <base/command_line.h>

#include "c_code_gen.h"
#include "cc_code_gen.h"

namespace insane {
namespace {
enum class CodegenTargets {
  None,
  CCodeGen,
  CXXCodeGen,
};

CodegenTargets GetTargetOption() {
  auto* current_commandline = base::CommandLine::ForCurrentProcess();
  if (current_commandline->FindSwitch(u8"target=cc"))
    return CodegenTargets::CXXCodeGen;
  // default to C
  return CodegenTargets::CCodeGen;
}
}  // namespace

// later you may be perhaps able to mix cc and c for one project, so we keep
// that door open by checking the target option for each file now, even if there
// is no way to specify that rn
base::UniquePointer<CodeGen> CreateCodeGenerator() {
  switch (GetTargetOption()) {
    default:
    case CodegenTargets::None:
      return {};
    case CodegenTargets::CCodeGen:
      return base::MakeUnique<CCodeGen>();
    case CodegenTargets::CXXCodeGen:
      // return base::MakeUnique<CCodeWriter>();
      return {};
  }
}

}  // namespace insane