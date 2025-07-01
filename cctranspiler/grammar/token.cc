// Copyright (C) The Fusion Authors/Vincent Hengel 2023

#include "token.h"

namespace aki {
namespace {
const char* const kTokenTypeToNames[] = {
    "invalid",                   // Invalid
    "invalid_number",            // InvalidNumber
    "eol",                       // Eol
    "eof",                       // Eof
    "semicolon",                 // Semicolon
    "colon",                     // Colon
    "colon_colon",               // ColonColon
    "comma",                     // Comma
    "l_paren",                   // LParen
    "r_paren",                   // RParen
    "l_curly",                   // LCurly
    "r_curly",                   // RCurly
    "l_square",                  // LSquare
    "r_square",                  // RSquare
    "dollar",                    // Dollar
    "tilde",                     // Tilde
    "hash",                      // Hash
    "dot",                       // Dot
    "plus",                      // Plus
    "minus",                     // Minus
    "asterisk",                  // Asterisk
    "forward_slash",             // ForwardSlash
    "equal",                     // Equal
    "greater_than",              // GreaterThan
    "less_than",                 // LessThan
    "not",                       // Not
    "ampersand",                 // Ampersand
    "pipe",                      // Pipe
    "caret",                     // Caret
    "percent_sign",              // PercentSign
    "question_mark",             // QuestionMark
    "plus_plus",                 // PlusPlus
    "plus_equals",               // PlusEquals
    "minus_minus",               // MinusMinus
    "minus_equals",              // MinusEquals
    "asterisk_equals",           // AsteriskEquals
    "forward_slash_equals",      // ForwardSlashEquals
    "double_equal",              // DoubleEqual
    "fat_arrow",                 // FatArrow
    "arrow",                     // Arrow
    "greater_than_or_equal",     // GreaterThanOrEqual
    "less_than_or_equal",        // LessThanOrEqual
    "not_equal",                 // NotEqual
    "ampersand_equals",          // AmpersandEquals
    "pipe_equals",               // PipeEquals
    "caret_equals",              // CaretEquals
    "percent_sign_equals",       // PercentSignEquals
    "logical_and",               // LogicalAnd
    "logical_or",                // LogicalOr
    "nullish_coalescing",        // NullishCoalescing
    "nullish_coalescing_equal",  // NullishCoalescingEquals
    "question_dot",              // QuestionDot
    "left_shift",                // LeftShift
    "right_shift",               // RightShift
    "left_shift_equals",         // LeftShiftEquals
    "right_shift_equals",        // RightShiftEquals
    "left_arithmetic_shift",     // LeftArithmeticShift
    "right_arithmetic_shift",    // RightArithmeticShift
    "range",                     // Range
    "ellipsis",                  // Ellipsis
    "integer_number",            // IntegerNumber
    "float_number",              // FloatNumber
    "bool_literal",              // BoolLiteral
    "quoted_string",             // QuotedString
    "quoted_string_u16",         // QuotedStringU16
    "quoted_string_u32",         // QuotedStringU32
    "template_literal",          // TemplateLiteral
    "identifier",                // Identifier
    "fn_keyword",                // FnKeyword
    "let_keyword",               // LetKeyword
    "const_keyword",             // ConstKeyword
    "if_keyword",                // IfKeyword
    "else_keyword",              // ElseKeyword
    "for_keyword",               // ForKeyword
    "while_keyword",             // WhileKeyword
    "return_keyword",            // ReturnKeyword
    "line_comment",              // LineComment
    "block_comment"              // BlockComment
};

static_assert(sizeof(kTokenTypeToNames) / sizeof(const char*) ==
                  static_cast<size_t>(TokenType::COUNT),
              "Mapping mismatch");
}  // namespace

const char* TokenTypeToName(TokenType type) noexcept {
  return kTokenTypeToNames[static_cast<size_t>(type)];
}

}  // namespace aki
