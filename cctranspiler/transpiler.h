// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023
#pragma once

#include <base/containers/vector.h>
#include <base/filesystem/file.h>
#include <base/filesystem/path.h>
#include <tbb/task_scheduler_observer.h>
#include <tbb/tbb.h>

namespace aki {

class CCTranspiler {
 public:
  CCTranspiler();

  using file_list = base::Vector<base::Path>;

  // work on batches of files at the same time
  void ProcessSourceFiles(const file_list& input_file_canidates);

  // Parse a selection of text
  void ParseText(const base::StringRefU8 text, const bool is_eval_mode = false);

 private:
  file_list loaded_files_;

  // oneapi::tbb::task_scheduler observer_;
};
}  // namespace insane
