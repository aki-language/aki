// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <base/memory/unique_pointer.h>

#include "code_gen.h"

namespace aki {
// Create a code emitter from a given backend
base::UniquePointer<CodeGen> CreateCodeGenerator();
}  // namespace insane
