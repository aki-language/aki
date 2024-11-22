// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023
#pragma once

#include <base/containers/vector.h>
#include <base/filesystem/file.h>
#include <base/filesystem/path.h>
#include <tbb/task_scheduler_observer.h>
#include <tbb/tbb.h>

#include <mutex>

#include "utils/file_writer.h"

namespace aki {

class CCTranspiler {
 public:
  CCTranspiler(const base::Path* optional_out_path);

  using file_list = base::Vector<base::Path>;

  // work on batches of files at the same time
  void ProcessSourceFilesBatch(const file_list& input_file_canidates);

  void EvaluateAkiCode(const base::StringRefU8 code);

 private:
  // Parse some text that might contain aki code
  void ProcessAndStageAkiCode(const base::Path& original_file,
                              const base::StringRefU8 text);

 private:
  file_list loaded_files_;
  FileWriter file_writer_;
  const base::Path* output_dir_;
  // files_mutex_
  std::mutex files_mutex_;

  // oneapi::tbb::task_scheduler observer_;
};
}  // namespace aki
