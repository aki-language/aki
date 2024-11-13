// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

#include <base/arch.h>
#include <base/containers/vector.h>
#include <base/logging.h>
#include <base/strings/string_ref.h>

#include <array>
#include <unordered_map>
#include <vector>

namespace insane {

// https://source.chromium.org/chromium/chromium/src/+/main:third_party/protobuf/src/google/protobuf/io/printer.h;l=184?q=printer&ss=chromium%2Fchromium%2Fsrc
class TextPrinter {
 public:
  TextPrinter(base::StringU8& buffer);
  ~TextPrinter();

  void PushIndent();
  void PopIndent();

  static constexpr char8_t kTokenMarker = u8'$';

  struct Substitution {
    base::StringRefU8 first;
    base::StringRefU8 second;
  };
  // Recursive function to create an array of substutionsn
  // usage:   printer_.Print(u8"#include \"$file_name$it\";\n",
  // Substitution{u8"file_name", import.target});
  template <typename... Args>
  void Print(const base::StringRefU8 text, const Args&... args) {
    constexpr auto size = sizeof...(Args);

    const Substitution substitutions[size] = {args...};
    return PrintStack(text, substitutions, size);
  }

  // 0 arg handler for msvc.
  void Print(const base::StringRefU8 text) { RawWrite(text); }

  // placeholder, value mapping
  void PrintStack(const base::StringRefU8& text,
                  const Substitution* substitutions, mem_size size);

  // add contents directly to the buffer
  void RawWrite(const base::StringRefU8 text);

  auto& buffer() const { return buffer_; }
  u32 intendation_level() const { return current_intendation_level_; }

 private:
  u32 current_intendation_level_;
  base::StringU8& buffer_;
};
}  // namespace insane
