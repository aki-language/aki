// Copyright (C) 2024 Vincent Hengel.
// For licensing information see LICENSE at the root of this distribution.
//
// Auto-generated file. Do not edit manually.

#pragma once

#include <base/knob.h>

namespace feature_flags {

extern base::Knob<bool> MangleTranslatedSymbols;
extern base::Knob<bool> EmitCompilerSpecificTypes;
extern base::Knob<bool> EmitAutoGenHeader;

inline void InitializeAllKnobs() {
  MangleTranslatedSymbols.Construct();
  EmitCompilerSpecificTypes.Construct();
  EmitAutoGenHeader.Construct();
}

inline void DestructAllKnobs() {
  MangleTranslatedSymbols.Destruct();
  EmitCompilerSpecificTypes.Destruct();
  EmitAutoGenHeader.Destruct();
}

struct KnobEntry {
  const char* name;
  base::BasicKnob* knob_obj;
};

constexpr int kKnobCount = 3;

inline void InitializeAllKnobsAndRegister(KnobEntry (&knob_list)[kKnobCount]) {
  InitializeAllKnobs();
  knob_list[0] =
      KnobEntry{"mangle_translated_symbols", &MangleTranslatedSymbols};
  knob_list[1] =
      KnobEntry{"emit_compiler_specific_types", &EmitCompilerSpecificTypes};
  knob_list[2] = KnobEntry{"emit_auto_gen_header", &EmitAutoGenHeader};
}

}  // namespace feature_flags
