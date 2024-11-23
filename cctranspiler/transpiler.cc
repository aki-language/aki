#include "transpiler.h"

#include <base/command_line.h>
#include <base/filesystem/file_util.h>
#include <base/filesystem/path.h>
#include <base/logging.h>
#include <base/text/code_point_validation.h>

#include "base/containers/vector.h"
#include "base/memory/unique_pointer.h"
#include "base/strings/string_ref.h"
#include "codegen/code_gen.h"
#include "codegen/code_gen_factory.h"
#include "grammar/lexer.h"
#include "grammar/parser.h"

namespace tbb::detail::r1 {
void do_throw_noexcept(void (*throw_exception)()) {}
}  // namespace tbb::detail::r1

namespace {
constexpr char kTag[] = "aki-transpiler";
}  // namespace

namespace aki {
static base::UniquePointer<byte[]> LoadAndValidateFile(
    const base::Path& file_path, i64* size) {
  if (!size) return nullptr;

  auto bytes = base::ReadFile(file_path, size);
  if (*size == 0) {
    BASE_LOG_ERROR("Failed to load file: {}", file_path.ToAsciiString());
    return nullptr;
  }

  const char8_t* data = reinterpret_cast<const char8_t*>(
      bytes.Get_UseOnlyIfYouKnowWhatYouareDoing());
  if (!base::DoIsStringUTF8(data, *size)) {
    BASE_LOG_ERROR("File is not UTF-8 encoded: {}", file_path.ToAsciiString());
    return nullptr;
  }

  return bytes;
}

static base::Path BuildOutputPath(const base::Path& out,
                                  const base::Path& og_aki_file_path) {
  auto fname = og_aki_file_path.BaseName().path();
  // find the extension and remove it
  auto pos = fname.find_last_of('.');
  if (pos == base::StringU8::npos) {
    // assume its a .aki file
    fname.remove_suffix(4);
  } else {
    fname.remove_suffix(fname.length() - pos);
  }
  return out / base::Path(u8"aki_" + fname + u8".c");
}

CCTranspiler::CCTranspiler() {}

void CCTranspiler::ProcessSourceFilesBatch(
    const file_list& input_file_candidates) {
  struct FileData {
    base::Path path;
    base::UniquePointer<byte[]> content;
    i64 size;
  };

  base::Vector<FileData> valid_files(input_file_candidates.size(),
                                     base::VectorReservePolicy::kForPushback);

  // Load files in parallel
  tbb::parallel_for_each(
      input_file_candidates.begin(), input_file_candidates.end(),
      [&](const base::Path& file_path) {
        std::printf("Processing file: %s\n", file_path.ToAsciiString().c_str());
        i64 size = 0;
        auto content = LoadAndValidateFile(file_path, &size);
        if (content) {
          std::lock_guard<std::mutex> lock(files_mutex_);
          valid_files.push_back({file_path, base::move(content), size});
        }
      });

  // Process files in parallel
  tbb::parallel_for_each(
      valid_files.begin(), valid_files.end(), [&](const FileData& file_data) {
        const char8_t* data = reinterpret_cast<const char8_t*>(
            file_data.content.Get_UseOnlyIfYouKnowWhatYouareDoing());
        const base::Path& file_path = file_data.path;

        ProcessAndStageAkiCode(file_path,
                               base::StringRefU8(data, file_data.size));
      });
}

void CCTranspiler::ProcessAndStageAkiCode(const base::Path& original_file,
                                          const base::StringRefU8 text) {
  aki::Lexer lexer;
  lexer.Parse(text);

  aki::Parser parser(base::move(lexer.tokens()));
  parser.ParseTokens();

  auto gen = CreateCodeGenerator();
  if (!gen) {
    BASE_LOGE(kTag, "Failed to find a code generator");
    return;
  }

  const auto result = gen->GenerateCode(parser.translation_unit());
  if (result != CodeGen::Result::Success) {
    BASE_LOGE(kTag, "Failed to generate code: {}", static_cast<int>(result));
    return;
  }

  // assemble a new output path by appending _aki to the original file name and
  // placing it in the output directory
  const auto new_path = BuildOutputPath(*output_dir_, original_file);

  // This is our build artifact
  const base::StringRefU8 code = gen->GetTextBuffer();
  file_writer_.EnqueueWrite(new_path, code);
  BASE_LOGI(kTag, "Enqueued file for writing: {}", new_path.ToAsciiString());
}

void CCTranspiler::EvaluateAkiCode(const base::StringRefU8 text) {
  aki::Lexer lexer;
  lexer.Parse(text);

  aki::Parser parser(base::move(lexer.tokens()));
  parser.ParseTokens();

  auto gen = CreateCodeGenerator();
  if (!gen) {
    BASE_LOGE(kTag, "Failed to find a code generator");
    return;
  }

  const auto result = gen->GenerateCode(parser.translation_unit());
  if (result != CodeGen::Result::Success) {
    BASE_LOGE(kTag, "Failed to generate code: {}", static_cast<int>(result));
    return;
  }

  // This is our build artifact
  const base::StringRefU8 code = gen->GetTextBuffer();
  std::puts(reinterpret_cast<const char*>(code.data()));
}

}  // namespace aki
