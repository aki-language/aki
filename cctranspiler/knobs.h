// Copyright (C) 2024 Vincent Hengel.
// For licensing information see LICENSE at the root of this distribution.
//
// Auto-generated file. Do not edit manually.

#pragma once

#include <base/knob.h>

namespace feature_flags {

extern base::Knob<bool> MangleTranslatedSymbols;

inline void InitializeAllKnobs() { MangleTranslatedSymbols.Construct(); }

inline void DestructAllKnobs() { MangleTranslatedSymbols.Destruct(); }

struct KnobEntry {
  const char* name;
  base::BasicKnob* knob_obj;
};

constexpr int kKnobCount = 1;

inline void InitializeAllKnobsAndRegister(KnobEntry (&knob_list)[kKnobCount]) {
  InitializeAllKnobs();
  knob_list[0] =
      KnobEntry{"mangle_translated_symbols", &MangleTranslatedSymbols};
}

}  // namespace feature_flags
