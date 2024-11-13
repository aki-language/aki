// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023

#include "transpiler.h"

#include <base/command_line.h>
#include <base/filesystem/file_util.h>
#include <base/filesystem/path.h>
#include <base/logging.h>
#include <base/text/code_point_validation.h>

#include "codegen/code_gen_factory.h"
#include "grammar/lexer.h"
#include "grammar/parser.h"

// NOTE(Vince): major hack, placed here for now to disable compilation of TBB
// without exceptions properly.
namespace tbb::detail::r1 {
void do_throw_noexcept(void (*throw_exception)()) {}
}  // namespace tbb::detail::r1

namespace {}  // namespace

namespace insane {
std::unique_ptr<byte[]> LoadFile(const base::Path& file_path) {
  i64 size = 0;
  auto bytes = base::LoadFile(file_path, &size);
  if (size == 0) {
    BASE_LOG_ERROR("Failed to load file: {}", file_path.ToAsciiString());
    return nullptr;
  }
  // make sure we only allow files with supported character types.
  // this should be really fast with ICU
  const char8_t* data = reinterpret_cast<const char8_t*>(bytes.get());
  if (!base::DoIsStringUTF8(data, size)) {
    BASE_LOG_ERROR("File is not UTF-8 encoded: {}", file_path.ToAsciiString());
    return nullptr;
  }
  return bytes;
}

InsaneTranspiler::InsaneTranspiler() {}

void InsaneTranspiler::ProcessSourceFiles(
    const file_list& input_file_canidates) {
  base::Vector<std::unique_ptr<byte[]>> file_contents;

  // IO is blocking regardless for now since we have to upload to common vector,
  // and i wanna keep that as a queue of sorts?
  for (const base::Path& file_path : input_file_canidates) {
    auto bytes = LoadFile(file_path);
    if (!bytes) continue;
    loaded_files_.push_back(file_path);
    file_contents.emplace_back(base::move(bytes));
  }

  // NOTE(Vince): We parse each file individually, and then we can do a second
  // pass.
  tbb::parallel_for((i64)0, static_cast<i64>(file_contents.size()), [&](i64 i) {
    const char8_t* data =
        reinterpret_cast<const char8_t*>(file_contents[i].get());
    ParseFile(data);
  });
}

void InsaneTranspiler::ParseFile(const base::StringRefU8 text) {
  // step 1: lexer is allowed to live on stack, only stores a vector.
  // this splits the text into tokens, without any syntax comprehension.
  insane::Lexer lexer;
  lexer.Parse(text);

  // step 2: parse the tokens into an AST.
  insane::Parser parser(base::move(lexer.tokens()));
  parser.ParseTokens();

  // for now, we just generate code individually for each unit.
  // we can do a second pass later to optimize this.
  auto gen = CreateCodeGenerator();
  gen->GenerateCode(parser.translation_unit());
}
}  // namespace insane