// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023
#pragma once

#include <base/containers/vector.h>

#include <base/filesystem/path.h>
#include <base/filesystem/file.h>

#include <tbb/task_scheduler_observer.h>
#include <tbb/tbb.h>

namespace insane {

class InsaneTranspiler {
 public:
  InsaneTranspiler();

  using file_list = base::Vector<base::Path>;

  void ProcessSourceFiles(const file_list& input_file_canidates);

 private:
  void ParseFile(const base::StringRefU8 text);

 private:
  file_list loaded_files_;

  //oneapi::tbb::task_scheduler observer_;
};
}  // namespace fusion
