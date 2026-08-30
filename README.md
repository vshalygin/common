# common

[![CI](https://github.com/vshalygin/common/actions/workflows/ci.yml/badge.svg)](https://github.com/vshalygin/common/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17%2B-00599C?logo=c%2B%2B)
![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake)

`common` is a collection of reusable static C++ libraries for applications
targeting Linux and Windows.

The project is under active development. C++17 is the minimum supported
language standard; consumers may select a newer standard for their build.

## Libraries

| Component | CMake target | Platforms | Purpose |
| --- | --- | --- | --- |
| [`common-lib`](common-lib/README.md) | `vshalygin::common-lib` | Windows, Linux | General-purpose, platform-independent utilities. Its facilities cover broad areas such as concurrency, synchronization, memory management, data handling, and compile-time programming. |
| [`rpc-lib`](rpc-lib/README.md) | `vshalygin::rpc-lib` | Windows, Linux | Asynchronous remote procedure calls over diverse, extensible transport implementations. |
| [`win-lib`](win-lib/README.md) | `vshalygin::win-lib` | Windows | Reusable components for functionality specific to the Windows platform. |

The repository also contains [`rpc-example`](example/rpc-example/README.md), a
small working application that demonstrates how to use `rpc-lib`. The example
is built as a standalone project and consumes the installed package through
`find_package()`, just like an external application.

## Highlights

- Static libraries with installable CMake package metadata.
- MSVC, GCC, and Clang support.
- x64 and x86 builds on Windows and Linux.
- Debug and Release presets, with interprocedural optimization in Release when
  supported by the toolchain.
- Recommended presets with dependencies managed through vcpkg manifest mode.
- Tests discovered and executed individually through GoogleTest and CTest.
- GCC line and branch coverage reports.
- Clang AddressSanitizer, UndefinedBehaviorSanitizer, and ThreadSanitizer
  configurations.
- GitHub Actions builds, tests, installs, and validates the standalone example.

## Requirements

- CMake 3.25 or newer.
- A C++17-capable MSVC, GCC, or Clang compiler. The provided presets and CI
  currently use Visual Studio 2022 with the v143 toolset, GCC 11, and Clang 14;
  these versions are recommended configurations rather than requirements for
  custom builds.
- [vcpkg](https://github.com/microsoft/vcpkg), bootstrapped locally, required
  for the provided presets.
- Ninja for the provided Linux presets.
- A multilib toolchain and 32-bit development libraries for Linux x86 builds.
- Python 3 and gcovr 8.6 for coverage generation.

The checked-in `vcpkg.json` pins the dependency recipes through a baseline and
declares Boost, Protobuf, and GoogleTest. The project presets locate the vcpkg
toolchain through the `VCPKG_ROOT` environment variable.

## Getting started

The [Getting started guide](docs/getting-started.md) provides a complete first
build for Windows or Linux, including dependency setup, tests, installation,
and the standalone RPC example.

For all presets, custom toolchains, dependency alternatives, and installation
options, see [Building and installing](docs/build-and-install.md).

## Using the installed package

Add the installation prefix to the consumer's CMake search path and import the
required target:

```cmake
find_package(vshalygin-common CONFIG REQUIRED)

add_executable(my-application main.cpp)
target_link_libraries(my-application PRIVATE vshalygin::rpc-lib)
```

For a command-line configuration:

```bash
cmake -S path/to/consumer -B path/to/build \
    -DCMAKE_PREFIX_PATH=/path/to/vshalygin-common
```

The exported targets propagate their public include directories and transitive
link dependencies. `vshalygin::win-lib` is exported only by Windows builds.

## Testing

See [Testing](docs/testing.md) for the test suites, local presets, coverage,
sanitizer workflows, and CI test execution.

## Continuous integration

GitHub Actions runs on pushes to `main`, pull requests targeting `main`, and
manual dispatches from any branch. The primary matrix contains 12 builds:

| Platform | Compilers | Architectures | Configurations |
| --- | --- | --- | --- |
| Linux | GCC 11, Clang 14 | x64, x86 | Debug, Release |
| Windows | MSVC v143 | x64, Win32 | Debug, Release |

The workflow validates the supported build matrix, package installation, and
the standalone consumer. Installed packages and example binaries are retained
as workflow artifacts. Test and diagnostic job behavior is documented in
[Testing](docs/testing.md).

Linux jobs use versioned Docker toolchain images published to GitHub Container
Registry. vcpkg binary caches are restored independently for compatible
toolchain and architecture combinations.

## Repository layout

```text
.
|-- common-lib/          Portable core library
|-- rpc-lib/             Asynchronous RPC library
|-- win-lib/             Windows-specific utilities
|-- example/             Standalone examples and installed-package consumers
|-- test/                GoogleTest test projects
|-- cmake/               Build, install, coverage, and sanitizer helpers
|-- docker/              Reproducible Linux CI toolchains
|-- docs/                Design and development documentation
|-- CMakePresets.json    Recommended local and CI configurations
`-- vcpkg.json           Dependency manifest and registry baseline
```

## Documentation

- [Documentation index](docs/README.md)
- [Getting started](docs/getting-started.md)
- [Building and installing](docs/build-and-install.md)
- [Testing](docs/testing.md)
- [Architecture](docs/architecture.md)
- [Future semantics](docs/future-semantics.md)
- [RPC architecture and protocol](docs/rpc-architecture.md)
- [Project roadmap](ROADMAP.md)
