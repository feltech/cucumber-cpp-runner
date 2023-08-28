# Cucumber C++ runner

[![ci](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/feltech/cucumber-cpp-runner/branch/main/graph/badge.svg)](https://codecov.io/gh/feltech/cucumber-cpp-runner)
[![CodeQL](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml)

## What

Execute C++ Gherkin tests with the Cucumber client and Cucumber-CPP server in a single `main`.

Internally uses the [cucumber wire protocol](https://github.com/cucumber/cucumber-ruby-wire) via a
`.wire` YAML configuration file, allowing the Ruby command-line client to execute C++ step
definitions.

```shell
Allowed options:
  -h [ --help ]                         this help
  -v [ --verbose ]                      verbose output
  -f [ --features ] arg (=.)            location of feature file(s)
  -c [ --cucumber ] arg (=cucumber)     cucumber executable
  -o [ --options ] arg                  additional cucumber options (surround 
                                        in quotes for multiple)
```

## Getting started

### Runtime dependencies

* [Ruby](https://www.ruby-lang.org/en/documentation/installation) (>2<3 for `cucumber` client)
* [cucumber](https://github.com/cucumber/cucumber-ruby) - Ruby Gem
* [cucumber-wire](https://github.com/cucumber/cucumber-ruby-wire) (>6) - Ruby Gem
* [Boost](https://www.boost.org/) (tested with 1.78)
* [Cucumber-CPP](https://github.com/cucumber/cucumber-cpp) (tested with main as of 2023-08-28)

### Build dependencies

#### Tooling

* CMake (>=25 for [workflow presets](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html#workflow-preset),
  see below).
* [Conan](https://conan.io/) (>=2) - optional, for installing dependencies in instructions below.

#### Libraries

* [Cucumber-CPP](https://github.com/cucumber/cucumber-cpp) (tested with main as of 2023-08-28) -
  optional, will be downloaded as part of build if not discovered.
* [Boost](https://www.boost.org/) (tested with 1.78) - see `conan` in instructions below.


### Linux/MacOS (tested on Ubuntu 20.04)
```shell
# Get cucumber command-line client
bundler install
# Install dependencies not downloaded as part of build.
conan install -of .conan 
export CMAKE_TOOLCHAIN_FILE=$(pwd)/.conan/conan_toolchain.cmake
# Build then install to `out/install`. Can switch "gcc" for "clang"; and/or "release" for "debug";
# and/or remove "-install".
cmake --workflow --preset unixlike-gcc-release-install
```

An optional [conda](https://conda.io/projects/conda/en/latest/glossary.html#miniconda-glossary) 
configuration file is available with known working build tooling on Ubuntu 20.04
```shell
conda env create cucumber-cpp-runner-build --file conda.yml
conda activate cucumber-cpp-runner-build
```

### Windows 10

As admin:

```DOS
: Note: Ruby in Conda is built with MSVC, but cucumber has a dependency on the `ffi` package, which
: is unavailable for MSVC builds. So using chocolatey to get an MSYS2 build of Ruby.
choco install ruby --version 2.7.7.1
```

Then:

```DOS
: For non-admin non-polluting install (must set GEM_PATH and PATH, though).
bundler install --binstubs --path .ruby
set GEM_PATH=%CD%\.ruby\ruby\2.7.0;%PATH%
set PATH=%CD%\.ruby\ruby\2.7.0\bin;%PATH%
# Install dependencies not downloaded as part of build.
conan install -of .conan 
set CMAKE_TOOLCHAIN_FILE=%CD%\.conan\conan_toolchain.cmake
# Build then install to `out/install`. Can switch "msvc" for "clang"; and/or "release" for "debug";
# and/or remove "-install".
cmake --workflow --preset windows-msvc-release-install
```