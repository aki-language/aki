// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023

#include <base/command_line.h>
#include <base/filesystem/file_util.h>
#include <base/logging.h>
#include <base/strings/xstring.h>

#include <cstdio>
#include <exception>

#include "base/arch.h"
#include "base/check.h"
#include "base/knob.h"
#include "base/strings/string_ref.h"
#include "knobs.h"
#include "transpiler.h"

#ifdef OS_WIN
#include <Windows.h>
#endif

namespace {

// Constants
constexpr char kSaneLogo[] = "akitrans v0.0.1 (c) 2024 Vincent Hengel";
constexpr char kUsageString[] = "Usage: akitrans [options] <file>\n";
constexpr char kHelpString[] = R"(Flags:

Options:
  -h, --help:       Print this help message
  -o, --output:     Specify the output directory
  -v, --verbose:    Print verbose output
  -e, --eval:       Evaluate the given expression

Arguments:
   FILES...         The files to compile)";

// Global state
bool mute_log = false;
bool verbose_logging = false;
bool eval_mode = false;
feature_flags::KnobEntry g_options[feature_flags::kKnobCount];

#ifdef OS_WIN
void ConfigureConsoleMode() {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
}
#else
void ConfigureConsoleMode() {}
#endif

void SetBaseHandlers() {
  base::SetLogHandler(
      [](void*, const char* channel_name, base::LogLevel log_level,
         const char* msg) {
        if (mute_log) return;
        if (!verbose_logging && (log_level == base::LogLevel::kDebug ||
                                   log_level == base::LogLevel::kTrace))
          return;

        std::printf("[%s]: %s\n", channel_name, msg);
      },
      nullptr);

  base::SetCheckHandler([](const char* message, const char* file_name,
                           const char* function, const char* msg) {
    auto location = fmt::format(fmt::runtime(message), file_name, function);
    std::printf("Bugcheck triggered in file %s!%s: %s. aborting.",
                location.c_str(), function, message);
    std::fflush(stdout);
    std::terminate();
  });
}

void SetKnobsFromCommandLine(base::CommandLine& command_line) {
  for (mem_size i = 0; i < feature_flags::kKnobCount; ++i) {
    const base::StringRefU8 knob_name((const char8_t*)g_options[i].name);
    const auto idx = command_line.FindSwitchIndex(knob_name);

    if (idx != base::CommandLine::kNotFoundIndex) {
      base::BasicKnob* knob_obj = g_options[i].knob_obj;
      const auto& value = command_line.ExtractSwitchValue(command_line.at(idx));

      auto* bool_knob = reinterpret_cast<base::Knob<bool>*>(knob_obj);
      if (value == u8"true" || value == u8"1") {
        bool_knob->set_value(true);
        *reinterpret_cast<bool*>(&knob_obj) = true;
      } else if (value == u8"false" || value == u8"0") {
        bool_knob->set_value(false);
      }
    }
  }
}

bool HandleCommandLineOptions(base::CommandLine& command_line) {
  if (command_line.parameter_count() < 2) {
    std::puts(kUsageString);
    return false;
  }

  eval_mode = command_line.FindSwitchWithAlias(u8"--eval", u8"-e");
  verbose_logging = command_line.FindSwitchWithAlias(u8"--verbose", u8"-v");

  if (!command_line.FindSwitch(u8"-nologo") && !eval_mode) {
    std::puts(kSaneLogo);
  }

  if (command_line.FindSwitchWithAlias(u8"--help", u8"-h")) {
    std::puts(kHelpString);
    return false;
  }

  if (command_line.FindSwitch(u8"-o") ||
      command_line.FindSwitch(u8"--output")) {
    BASE_LOG_ERROR("Output directory not yet supported");
    return false;
  }

  return true;
}

bool HandleEvalMode(base::CommandLine& command_line,
                    insane::InsaneTranspiler& app) {
  if (!eval_mode) return true;

  const auto idx = command_line.FindSwitchIndex(u8"eval");
  if (idx == base::CommandLine::kNotFoundIndex) {
    BASE_LOG_ERROR("No expression provided for eval mode");
    return false;
  }

  // shut up the logs and header generation
  feature_flags::EmitAutoGenHeader.set_value(false);
  mute_log = true;

  const base::StringRefU8 text = command_line[idx];
  auto eval_code = text.substr(7, text.length() - 1); // this is a bit cursed

  app.ParseText(eval_code, true);
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  ConfigureConsoleMode();
  SetBaseHandlers();

  feature_flags::InitializeAllKnobsAndRegister(g_options);

#ifdef OS_WIN
  base::CommandLine command_line;
#else
  base::CommandLine command_line(argc, argv);
#endif

  if (!HandleCommandLineOptions(command_line)) {
    return 0;
  }

  SetKnobsFromCommandLine(command_line);
  auto app = base::MakeUnique<insane::InsaneTranspiler>();

  if (!HandleEvalMode(command_line, *app)) {
    return 0;
  }

  const auto positional_index = command_line.FindPositionalArgumentsIndex();
  insane::InsaneTranspiler::file_list input_source_paths;

  for (mem_size i = positional_index; i < command_line.parameter_count(); i++) {
    if (!base::PathExists(command_line[i])) {
      BASE_LOG_ERROR("File does not exist: {}",
                     (const char*)command_line[i].data());
      return 1;
    }
    input_source_paths.push_back(command_line[i]);
  }

  app->ProcessSourceFiles(input_source_paths);
  return 0;
}
