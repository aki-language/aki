// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "token.h"

namespace insane {
namespace {
const char* const kTokenTypeToNames[] = {"unknown",
                                         "semicolon",
                                         "colon",
                                         "colon_colon",
                                         "plus",
                                         "plus_plus",
                                         "plus_equals",
                                         "minus",
                                         "minus_minus",
                                         "minus_equals",
                                         "asterisk_equal",
                                         "asterisk",
                                         "forward_slash",
                                         "forward_slash_equal",
                                         "equal",
                                         "double_equal",
                                         "fat_arrow",
                                         "greater_than_or_equal",
                                         "right_shift_equal",
                                         "right_arithmetic_shift",
                                         "right_shift",
                                         "greater_than",
                                         "less_than_or_equal",
                                         "left_shift_equal",
                                         "left_arithmetic_shift",
                                         "left_shift",
                                         "less_than",
                                         "not_equal",
                                         "exclamation_point",
                                         "ampersand_equal",
                                         "ampersand",
                                         "pipe_equal",
                                         "pipe",
                                         "caret_equal",
                                         "caret",
                                         "dollar",
                                         "tilde",
                                         "hash",
                                         "percent_sign_equal",
                                         "percent_sign",
                                         "question_mark_question_mark_equal",
                                         "question_mark_question_mark",
                                         "question_mark",
                                         "comma",
                                         "dot",
                                         "dot_dot",
                                         "dot_dot_dot",
                                         "l_paren",
                                         "r_paren",
                                         "l_curly",
                                         "r_curly",
                                         "l_square",
                                         "r_square",
                                         "eol",
                                         "eof",
                                         "quoted_string",
                                         "quoted_string_u16",
                                         "quoted_string_u32",
                                         "character_sequence",
                                         "comment",
                                         "number",
                                         "floating_number",
                                         "binary_number",
                                         "hex_number",
                                         "octal_number"};
}

const char* TokenTypeToName(TokenType type) noexcept {
  return kTokenTypeToNames[static_cast<size_t>(type)];
}

static_assert(sizeof(kTokenTypeToNames) / sizeof(const char*) ==
                  static_cast<size_t>(TokenType::COUNT),
              "Mapping mismatch");
}  // namespace insane
