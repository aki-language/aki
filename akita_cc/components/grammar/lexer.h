// Copyright (C) Vincent Hengel 2023-2025
#pragma once

#include <base/arch.h>
#include <base/containers/vector.h>
#include <base/strings/string_ref.h>
#include <base/strings/xstring.h>

#include "api.h"
#include "token.h"

namespace aki {
using TokenList = base::Vector<Token>;

AKI_GRAMMAR_API bool LexString(TokenList& tokens_out,
                               const base::StringRefU8 text);

AKI_GRAMMAR_API void LexIdentifier(TokenList& tokens,
                                   const base::StringRefU8 text,
                                   mem_size& index);

AKI_GRAMMAR_API bool LexNumber(TokenList& tokens,
                               const base::StringRefU8 text,
                               mem_size& index,
                               mem_size custom_start,
                               bool has_leading_dot);

AKI_GRAMMAR_API void LexQuotedString(TokenList& tokens,
                                     const base::StringRefU8 text,
                                     mem_size& index,
                                     TokenType string_type,
                                     mem_size opener_len = 1);

}  // namespace aki
