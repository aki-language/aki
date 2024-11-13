// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023

#include <base/logging.h>
#include <base/command_line.h>
#include <base/strings/xstring.h>
#include <base/filesystem/file_util.h>
#include <cstdio>
#include <exception>

#include "base/arch.h"
#include "base/check.h"
#include "base/knob.h"
#include "base/strings/string_ref.h"
#include "transpiler.h"

#include "knobs.h"

#if defined(OS_WIN)
#include <Windows.h>
#endif

namespace {
constexpr char kSaneLogo[] = "akitrans v0.1.0 (c) 2023 Vincent Hengel";

constexpr char kUseageString[] = "Usage: akitrans [options] <file>\n";

constexpr char kHelpString[] =
    R"(Flags:

Options:
  -h, --help:       Print this help message
  -o, --output:     Specify the output directory
  -v, --verbose:    Print verbose output

Arguments:
   FILES...         The files to compile)";

void ConfigureConsoleMode() {
#if defined(OS_WIN)
  SetConsoleOutputCP(CP_UTF8);
  // Set the input code page to UTF-8
  SetConsoleCP(CP_UTF8);
#endif
}

bool wants_trace_and_debug = false;
feature_flags::KnobEntry options[feature_flags::kKnobCount];

void SetKnobsFromCommandLine(base::CommandLine& command_line) {
  for (mem_size i = 0; i < feature_flags::kKnobCount; ++i) {
    const base::StringRefU8 knob_name((const char8_t*)options[i].name);
    const auto idx = command_line.FindSwitchIndex(knob_name);
    if (idx != base::CommandLine::kNotFoundIndex) {
      base::BasicKnob* knob_obj = options[i].knob_obj;
      const auto& value = command_line.ExtractSwitchValue(command_line.at(idx));
      if (value == u8"true" || value == u8"1") {
        reinterpret_cast<base::Knob<bool>*>(knob_obj)->set_value(true);
        *reinterpret_cast<bool*>(&knob_obj) = true;
      } else if (value == u8"false" || value == u8"0") {
        reinterpret_cast<base::Knob<bool>*>(knob_obj)->set_value(false);
      }
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  ConfigureConsoleMode();

  // Handle log messages.
  base::SetLogHandler(
      [](void*, const char* channel_name, base::LogLevel log_level,
         const char* msg) {
        // drop log content we don't desire
        if (!wants_trace_and_debug && (log_level == base::LogLevel::kDebug ||
                                       log_level == base::LogLevel::kTrace))
          return;

        std::printf("[%s]: %s\n", channel_name, msg);
      },
      nullptr);

  // Similar to assert - Handles bugchecks, terminations etc.
  base::SetCheckHandler([](const char* message, const char* file_name,
                           const char* function, const char* msg) {
    auto location = fmt::format(fmt::runtime(message), file_name, function);
    std::printf("Bugcheck triggered in file %s!%s: %s. aborting.",
                location.c_str(), function, message);
    std::fflush(stdout);
    std::terminate();
  });

  // Bind the known global options (Knobs) so we can populate them via cmdl
  // below.
  feature_flags::InitializeAllKnobsAndRegister(options);

#if defined(OS_WIN)
  base::CommandLine
      command_line;  // windows doesnt need the params, we fetch at runtime.
#elif defined(OS_POSIX)
  base::CommandLine command_line(argc, argv);
#endif
  if (command_line.parameter_count() < 2) {
    std::puts(kUseageString);
    return 0;
  }

  if (!command_line.FindSwitch(u8"-nologo"))
    std::puts(&kSaneLogo[1]);

  if (command_line.FindSwitch(u8"-h") || command_line.FindSwitch(u8"--help")) {
    std::puts(kHelpString);
    return 0;
  }

  if (command_line.FindSwitch(u8"-v") ||
      command_line.FindSwitch(u8"--verbose")) {
    wants_trace_and_debug = true;
  }

  bool has_output_dir = false;
  if (command_line.FindSwitch(u8"-o") ||
      command_line.FindSwitch(u8"--output")) {
    BASE_LOG_ERROR("Output directory not yet supported");

    has_output_dir = true;
    return 0;
  }

  SetKnobsFromCommandLine(command_line);

  mem_size positional_index = 1;
  for (auto i = 1; i < command_line.parameter_count(); i++) {
    if (command_line[i].data()[0] == u8'-' ||
        (i + 1 < command_line.parameter_count() &&
         command_line[i + 1].data()[0] == u8'-')) {
      positional_index++;
    }
  }

  // now just follows a loose list of files, seperated by spaces.
  insane::InsaneTranspiler::file_list input_source_paths;
  for (mem_size i = positional_index; i < command_line.parameter_count(); i++) {
    // const base::Path u8path(base::)
    // fmt::print("LEO command: ", command_line[i]);
    if (!base::PathExists(command_line[i])) {
      BASE_LOG_ERROR("File does not exist: {}",
                     (const char*)command_line[i].data());
      return 1;
    }
    input_source_paths.push_back(command_line[i]);
  }

  // keep stack free
  auto app{base::MakeUnique<insane::InsaneTranspiler>()};
  app->ProcessSourceFiles(input_source_paths);
  return 0;
}
