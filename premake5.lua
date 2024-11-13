-- Copyright (C) 2022 Vincent Hengel.
-- For licensing information see LICENSE at the root of this distribution.
-- This is a build definition for the legacy c++ transpiler.

aki = {}
aki.rootdir = os.getcwd()
aki.extdir = fusion.rootdir .. "/extern"

grouped_include("extern", "aki/extern")
grouped_include("transpiler/cc", "aki/cctranspiler")
grouped_include("docs", "aki/docs")
grouped_include("samples", "aki/samples")
