-- Copyright (C) 2022 Vincent Hengel.
-- For licensing information see LICENSE at the root of this distribution.

local function build_transpiler()
    include_meta()
    files({
      "**.h",
      "**.cc"
    })
    dependencies({
      "fmtlib",
      "base",
      "tbb",
      "simdjson"
    })
    blu.include_root()
    include_eq_components()
    includedirs({
      ".",
      "../",
      "../../docs",
    })
    --defines("FMT_HEADER_ONLY")
end

project("akitrans")
  kind("ConsoleApp")
  build_transpiler()
  strip_testfiles()
  dependencies("fmtlib")

unittest2("akitrans:test")
  build_transpiler()
  removefiles({
    "main.cc",
    --"build_info.rc"
  })
