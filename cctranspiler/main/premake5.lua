-- Copyright (C) 2022-2025 Vincent Hengel.
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

project("akits_cc")
  kind("ConsoleApp")
  build_transpiler()
  strip_testfiles()
  dependencies("fmtlib")

unittest2("akits_cc:test")
  build_transpiler()
  removefiles({
    "main.cc",
    --"build_info.rc"
  })
