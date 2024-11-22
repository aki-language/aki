// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

namespace aki {

class CCTranspiler;

class SyntaxChecker {
 public:
  explicit SyntaxChecker(CCTranspiler& t) : transpiler_(t) {}

 private:
  CCTranspiler& transpiler_;
};
}  // namespace insane
