// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "keyword.h"

namespace aki {
namespace {
inline bool StringMatchSafe(const char8_t* lhs, const char8_t* rhs,
                            mem_size limit, mem_size& length) {
  char8_t c1, c2;

  while (limit-- && (c1 = *lhs++) == (c2 = *rhs++)) {
    length++;
    if (c1 == 0) return 0;
  }

  // We can't just return c1 - c2, because the difference might be greater than
  // INT_MAX.
  return (static_cast<u8>(c1) - static_cast<u8>(c2)) == 0;
}
}  // namespace

constexpr const char8_t* kKeyWords[] = {
    // core keywords
    u8"as",
    u8"is",  // reserved
    u8"isa",
    u8"let",
    u8"var",
    u8"raw",
    u8"enum",
    u8"class",
    u8"struct",
    u8"complex",
    u8"extends",
    u8"using",
    u8"import",
    u8"include",  // reserved
    u8"extern",
    u8"fn",
    u8"return",
    u8"public",
    u8"private",  // only works for access control in classes
    u8"switch",
    u8"match",
    u8"if",
    u8"for",
    u8"while",
    u8"break",
    u8"continue",
    // coupling
    u8"namespace",
    // boolean
    u8"bool",
    u8"true",
    u8"false",
    u8"TRUE",
    u8"FALSE",
    // numeric sizes
    u8"i8",
    u8"u8",
    u8"i16",
    u8"u16",
    u8"i32",
    u8"u32",
    u8"i64",
    u8"u64",
    u8"i128",
    u8"u128",
    // floats
    u8"f32",
    u8"f64",
    u8"f128",
    // characters
    u8"c8",
    u8"c16",
    u8"c32",

    u8"unknown",
};

const char8_t* KeywordToName(KeywordType type) noexcept {
  return kKeyWords[static_cast<size_t>(type)];
}

KeywordType MatchKeyword(const base::StringRefU8 ref) noexcept {
  for (size_t i = 0; i < (sizeof(kKeyWords) / sizeof(const char8_t*)); i++) {
    mem_size cc = 0;
    if (StringMatchSafe(ref.data(), kKeyWords[i], ref.length(), cc))
      return static_cast<KeywordType>(i);
  }

  return KeywordType::Unknown;
}

static_assert(sizeof(kKeyWords) / sizeof(const char8_t*) ==
                  static_cast<size_t>(KeywordType::COUNT),
              "Keyword mapping mismatch");
}  // namespace aki
