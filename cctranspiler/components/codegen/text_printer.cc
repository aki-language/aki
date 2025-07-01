// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "text_printer.h"

#include <base/check.h>
#include <base/containers/vector.h>
#include <base/text/utf8_codepoint_iterator.h>

#include "base/arch.h"
#include "base/strings/string_ref.h"

namespace aki {
TextPrinter::TextPrinter(base::StringU8& buffer)
    : current_intendation_level_(0), buffer_(buffer) {}

TextPrinter::~TextPrinter() {}

void TextPrinter::PushIndent() { current_intendation_level_ += 2; }

void TextPrinter::PopIndent() {
  if (current_intendation_level_ >= 2) {
    current_intendation_level_ -= 2;
  } else {
    BASE_LOG_WARNING("Attempted to pop indentation when there was none.");
  }
}

void TextPrinter::PrintStack(const base::StringRefU8& text,
                             const Substitution* substitutions, mem_size size) {
  base::StringU8 indented_text;
  indented_text.reserve(text.size() * 2);  // Preallocate memory

  // Add indentation
  indented_text.append(current_intendation_level_, u8' ');

  auto seek = [substitutions, size](
                  const base::StringRefU8& data) -> const base::StringRefU8* {
    for (mem_size i = 0; i < size; ++i) {
      if (substitutions[i].first == data) return &substitutions[i].second;
    }
    return nullptr;
  };

  for (mem_size i = 0; i < text.size(); ++i) {
    if (text[i] == kTokenMarker) {
      mem_size j = i + 1;
      while (j < text.size() && text[j] != kTokenMarker) {
        ++j;
      }

      if (j < text.size()) {
        base::StringU8 placeholder(text.data() + i + 1, j - i - 1);
        const base::StringRefU8* it = seek(placeholder);
        if (it != nullptr) {
          const auto& replacement_bit = *it;
          base::StringU8 replacement(replacement_bit.data(),
                                     replacement_bit.length() + 1);
          replacement[replacement.length() - 1] = 0;
          indented_text.append(replacement.c_str());
        } else {
          indented_text.push_back(kTokenMarker);
          indented_text.append(placeholder);
          indented_text.push_back(kTokenMarker);
        }

        i = j;
      } else {
        indented_text.push_back(kTokenMarker);
        break;
      }
    } else {
      indented_text.push_back(text[i]);
      if (text[i] == u8'\n' && i + 1 < text.size()) {
        indented_text.append(current_intendation_level_, u8' ');
      }
    }
  }
  buffer_.append(indented_text);
}

// we can only really have UTF8 characters in comments??
void TextPrinter::RawWrite(const base::StringRefU8 text) {
  buffer_.append(text.c_str(), text.length());
}
}  // namespace aki
