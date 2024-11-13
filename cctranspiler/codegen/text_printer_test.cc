
#include "gtest/gtest.h"
#include "text_printer.h"
#include <base/strings/string_compare.h>

namespace {
using namespace insane;

TEST(TextPrinterTest, PushAndPopIndent) {
  base::StringU8 buffer;
  TextPrinter printer(buffer);
  EXPECT_EQ(0, printer.intendation_level());
  printer.PushIndent();
  EXPECT_EQ(2, printer.intendation_level());
  printer.PushIndent();
  EXPECT_EQ(4, printer.intendation_level());
  printer.PopIndent();
  EXPECT_EQ(2, printer.intendation_level());
  printer.PopIndent();
  EXPECT_EQ(0, printer.intendation_level());
  printer.PopIndent();  // should not log warning
}

TEST(PrinterTest, SingleVariableSubstitution) {
  base::StringU8 buffer;
  TextPrinter printer(buffer);
  const TextPrinter::Substitution replacements[] = {
      {u8"bit_field_name", u8"MyBitFieldName"},
      {u8"type_name", u8"int"},
  };
  auto expectedOutput = u8"private int MyBitFieldName;\n";
  printer.PrintStack(u8"private $type_name$ $bit_field_name$;\n", replacements, 2);
  EXPECT_TRUE(base::Strcmp(expectedOutput, printer.buffer().c_str()) == 0);
}
}  // namespace
