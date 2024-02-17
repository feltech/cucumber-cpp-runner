# Cucumber C++ runner

[![ci](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/feltech/cucumber-cpp-runner/branch/main/graph/badge.svg)](https://codecov.io/gh/feltech/cucumber-cpp-runner)
[![CodeQL](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/feltech/cucumber-cpp-runner/actions/workflows/codeql-analysis.yml)

## What

Execute C++ [Gherkin](https://cucumber.io/docs/gherkin/) BDD tests with the
[Cucumber](https://github.com/cucumber/cucumber-ruby) client and
[Cucumber-Cpp](https://github.com/cucumber/cucumber-cpp) server in a single `main`.

## Why

[Cucumber-Cpp](https://github.com/cucumber/cucumber-cpp) works by running a local server, responding to requests via the
[cucumber-ruby-wire](https://github.com/cucumber/cucumber-ruby-wire) protocol. Actual discovery of
the `.feature` files, execution and reporting is then performed using the `cucumber` (Ruby)
command-line client. The client, on discovering a `.wire` config file in the `step_definitions`
directory, will attempt to execute steps via the wire server specified in the `.wire` file.

This poses a problem for test automation and debuggability. For example, it is not currently
possible to have CMake's CTest set up and tear down a daemon server as a fixture - it will hang
indefinitely waiting for the server to quit before proceding to execute the client.

So wrapping the client and server execution together in some way seems inevitable.

Other approaches could be taken, such as shell or Ruby scripts, but these are inherently harder to
hook into with a C++ debugger - all too important when developing tests.

A C++ wrapper enables the test and server executables to be one and the same, so analysing a test
run in a debugger becomes trivial.

## Getting started

### Runtime dependencies

* [Ruby](https://www.ruby-lang.org/en/documentation/installation) (>=2<3 for `cucumber` client)
* [cucumber](https://github.com/cucumber/cucumber-ruby) - Ruby Gem
* [cucumber-wire](https://github.com/cucumber/cucumber-ruby-wire) (>=6) - Ruby Gem
* [Boost](https://www.boost.org/) (tested with 1.78 - also a dependency of Cucumber-Cpp)
* [Cucumber-Cpp](https://github.com/cucumber/cucumber-cpp) (v0.7.0)

### Usage

See [Cucumber-Cpp](https://github.com/cucumber/cucumber-cpp) - the usage is very similar, the main
difference is that cucumber-cpp-runner will take care of executing the `cucumber` client, and will
auto-configure the server based on the `.wire` file discovered in the `step_definitions` directory.
 
Two CMake targets are exported

* `cucumber-cpp-runner::cucumber-cpp-runner` - linking this target to your Cucumber-Cpp step
  definitions library adds a `main` command-line application (see `--help` text below). This starts
  your server as defined in a `.wire` config file, then launches the `cucumber` command-line client
  to discover and execute your `.feature` files.
    ```shell
    Allowed options:
      -h [ --help ]                         this help
      -v [ --verbose ]                      verbose output
      -f [ --features ] arg (=.)            location of feature file(s)
      -c [ --cucumber ] arg (=cucumber)     cucumber executable
      -o [ --options ] arg                  additional cucumber options (surround 
                                            in quotes for multiple)
    ```
* `cucumber-cpp-runner::cucumber-cpp-runner-nomain` - linking this target eschews the `main`, allowing
  you to call `cucumber_cpp_runner::execute_cucumber_tests(...)` (found in
  `cucumber-cpp-runner/cucumber_cpp_runner.hpp`) from your own application, passing a configuration
  object:
    ```c++
    struct CucumberRunnerParams
    {
    /// Cucumber command-line executable - absolute or on PATH.
    fs::path cucumber_exe{"cucumber"};
    /// Additional command-line arguments to pass to cucumber executable.
    std::string cucumber_options{};
    /// Location of features directory, or a specific feature file.
    fs::path feature_path{"."};
    /// Enable some verbose cucumber-cpp-runner-specific logs.
    bool verbose{false};
    };
    ```

## Building

### Build dependencies

#### Tooling

* CMake (>=25 for [workflow presets](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html#workflow-preset),
  see below).
* Ninja (Linux and MacOS - if using CMake presets)
* [Conan](https://conan.io/) (>=2) - optional, for installing dependencies in instructions below.

#### Libraries

* [Cucumber-CPP](https://github.com/cucumber/cucumber-cpp) (tested with main as of 2023-08) -
    - Will be downloaded as part of build if not discovered.
* [Boost](https://www.boost.org/) (tested with 1.78)
    - See `conan` in instructions below.
* [fmt](https://github.com/fmtlib/fmt) (tested with 9.1.0) -
    - Will be downloaded as part of build if not discovered.
* [yaml-cpp](https://github.com/jbeder/yaml-cpp) (tested with 0.8.0) -
    - Will be downloaded as part of build if not discovered.

### Linux/MacOS (tested on Ubuntu 20.04)

```shell
# Get cucumber command-line client
bundler install
# Install dependencies not downloaded as part of build.
conan install -of .conan .
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

> Note: Ruby in Conda (used in Linux instructions above) is built with MSVC, but cucumber has a
> dependency on the `ffi` package, which is unavailable for MSVC builds. So below we use
[Chocolatey](https://docs.chocolatey.org) to get an MSYS2 build of Ruby.

```DOS
choco install ruby --version 2.7.7.1
```

Then as normal user:

```DOS
: For non-admin non-polluting install (must set GEM_PATH and PATH, though).
bundler install --binstubs --path .ruby
set GEM_PATH=%CD%\.ruby\ruby\2.7.0
set PATH=%CD%\.ruby\ruby\2.7.0\bin;%PATH%
: Install dependencies not downloaded as part of build.
conan install -of .conan .
set CMAKE_TOOLCHAIN_FILE=%CD%\.conan\conan_toolchain.cmake
: Build then install to `out/install`. Can switch "msvc" for "clang"; and/or "release" for "debug";
: and/or remove "-install".
cmake --workflow --preset windows-msvc-release-install
```