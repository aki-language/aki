// Copyright (C) The Fusion Authors/Vincent Hengel 2023
#pragma once

namespace insane {

class Transpiler;

class SyntaxChecker {
 public:
  explicit SyntaxChecker(Transpiler& t) : transpiler_(t) {}

 private:
  Transpiler& transpiler_;
};
}  // namespace insane
