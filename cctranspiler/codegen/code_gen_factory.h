// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include "code_gen.h"
#include <base/memory/unique_pointer.h>

namespace insane {
// Create a code emitter from a given backend
base::UniquePointer<CodeGen> CreateCodeGenerator();
}  // namespace fusion