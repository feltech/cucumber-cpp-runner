# Cucumber C++ runner

[![ci](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/feltech/cucumber-cpp-runner/branch/main/graph/badge.svg)](https://codecov.io/gh/feltech/cucumber-cpp-runner)
[![CodeQL](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml)

## What

Execute Gherkin tests with Cucumber client and Cucumber-CPP server in a single `main`.

## Getting started

### Dependencies

* CMake >=25

### Linux/MacOS (tested on Ubuntu 20.04)
```shell
# Optional - create dev environment. Must have conda-forge as primary channel:
conda env create cucumber-cpp-runner-build --file conda.yml
conda activate cucumber-cpp-runner-build
# Get cucumber command-line client
bundler install
# Install dependencies not downloaded as part of build.
conan install -of .conan 
export CMAKE_TOOLCHAIN_FILE=$(pwd)/.conan/conan_toolchain.cmake
# Build then install to `out/install` - can switch "gcc" for "clang"; and/or "release" for "debug";
# and/or remove "install".
cmake --workflow --preset unixlike-gcc-release-install
```


### Windows 10

```DOS
: As admin.
: Note: conda ruby is built with MSVC, but cucumber has dependency on `ffi` package, which is
: unavailable for MSVC builds and requires an MSYS2 build of ruby. Hence using chocolatey.
choco install ruby --version 2.7.7.1
: For non-admin non-polluting install (must add .ruby\ruby\2.7.0 to GEM_PATH and 
: .ruby\ruby\2.7.0\bin to PATH, though).
bundler install --binstubs --path .ruby
# Install dependencies not downloaded as part of build.
conan install -of .conan 
set CMAKE_TOOLCHAIN_FILE=%cd%/.conan/conan_toolchain.cmake
# Build then install to `out/install` - can switch "msvc" for "clang"; and/or "release" for "debug";
# and/or remove "install".
cmake --workflow --preset windows-msvc-release-install
```