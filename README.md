# Cucumber C++ runner

[![ci](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/feltech/cucumber-cpp-runner/branch/main/graph/badge.svg)](https://codecov.io/gh/feltech/cucumber-cpp-runner)
[![CodeQL](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml)

Execute Gherkin tests with Cucumber client and Cucumber-CPP server in a single `main`.

## Getting started

### Linux (Ubuntu 20.04)
```shell
# Create dev environment. Must have conda-forge as primary channel:
conda env create cucumber-cpp-runner-build --file conda.yml
conda activate cucumber-cpp-runner-build
# Get cucumber command-line client
bundler install
# Install dependencies not downloaded as part of build.
conan install -if .conan
# Build a release build, disabling linting. Add -Dcucumber_cpp_runner_ENABLE_TESTS to build tests.
cmake -S . -B build \
--preset unixlike-gcc-release\
-DCMAKE_PREFIX_PATH=.conan
# Install CMAKE_PREFIX_PATH-friendly dist in "out/install/unixlike-gcc-release".
cmake --install build
```


### Windows 10

```DOS
: As admin.
: Note: conda ruby is built with MSVC, but cucumber has dependency on `ffi` package, which is unavailable for MSVC
: builds and requires an MSYS2 build of ruby. Hence using chocolatey.
choco install ruby --version 2.7.7.1
: For non-admin non-polluting install (must add .ruby\ruby\2.7.0 to GEM_PATH and .ruby\ruby\2.7.0\bin to PATH for tests 
: though)
bundler install --binstubs --path .ruby
# Rest similar to Linux...
```