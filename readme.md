# Aki Programming Language

A modern, programming language designed for simplicity and efficiency.

⚠️ **Note:** This project is currently in development and not ready for production use.

## Overview

Aki is a statically-typed programming language that transpiles to C and C++, enabling wide platform compatibility. The language prioritizes:

- **Simplicity**: Easy to learn and use
- **Readability**: Clean, intuitive syntax
- **Performance**: Efficient compilation to native code
- **Portability**: Runs anywhere C/C++ is supported

## Current State

- ✅ Transpiler: Available
- 🚧 Compiler: In development

Currently the transpiler is written in C++ with a bunch of custom libraries. The goal is to gradually replace all of it with pure aki.

## Directory Structure

- `compiler/`: Aki compiler, written in Aki - heavily WIP - this is dogfooding the language
- `cctranspiler/`: Aki transpiler, written in C++ (Compiles aki to c/c++)
- `akikit/`: Aki standard library
- `examples/`: Example Aki programs [TBD]

## Documentation

Tutorial and guides available on the website: [aki.vhtek.net](https://aki.vhtek.net)
