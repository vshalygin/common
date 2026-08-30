# Building and installing

This guide explains how to configure, build, test, and install `common`, and
how to consume the resulting CMake package from another project.

The checked-in presets are the recommended path for local development and are
the configurations exercised by CI. They are not the only supported way to
build the libraries: regular CMake options may be used to select another
compatible compiler, language standard, dependency installation, architecture,
or installation prefix.

## Build model

The root project produces static library targets:

- `common-lib` on Linux and Windows;
- `rpc-lib` on Linux and Windows;
- `win-lib` on Windows only.

Tests are included when `BUILD_TESTING` is enabled. Projects under `example/`
are not part of the root build; they are configured separately after the
libraries have been installed.

All generated files, build products, manifest-installed dependency trees, and
installation outputs are kept outside the source directories under `out/` by
the provided presets. vcpkg's shared download cache remains under the selected
vcpkg root.

## Prerequisites

Every build requires:

- CMake 3.25 or newer;
- a C++17-capable MSVC, GCC, or Clang compiler;
- Boost 1.86 or newer with Boost.Thread;
- Protobuf with the `protobuf::libprotobuf` and `protobuf::protoc` CMake
  targets;
- platform thread support.

GoogleTest is required only when `BUILD_TESTING=ON`. The recommended Linux
presets use Ninja. Linux x86 builds additionally require a multilib compiler
and 32-bit system development libraries.

The current CI toolchains are MSVC v143, GCC 11, and Clang 14. These versions
define the continuously tested configurations, not a hard requirement for a
custom build.

## Providing dependencies with vcpkg

The recommended presets use vcpkg in manifest mode. Clone and bootstrap vcpkg
once, then set `VCPKG_ROOT` to its absolute path.

### Windows

```powershell
git clone https://github.com/microsoft/vcpkg.git C:/src/vcpkg
C:/src/vcpkg/bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = "C:/src/vcpkg"
```

### Linux

```bash
git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/vcpkg"
```

During CMake configuration, the vcpkg toolchain reads the root `vcpkg.json`,
resolves recipes at its pinned baseline, and installs the required packages for
the selected target triplet. The installed dependency tree is local to the
CMake build directory.

The default Windows presets select static vcpkg libraries and the static MSVC
runtime. Linux presets select the standard static vcpkg library triplets;
system runtime libraries remain platform-provided.

## Listing available presets

Run these commands from the repository root:

```bash
cmake --list-presets=configure
cmake --list-presets=build
cmake --list-presets=test
cmake --list-presets=workflow
```

The principal build configurations are:

| Platform | Compiler | Architecture | Configurations |
| --- | --- | --- | --- |
| Windows | MSVC | x64, Win32 | Debug, Release |
| Linux | GCC | x64, x86 | Debug, Release |
| Linux | Clang | x64, x86 | Debug, Release |

Coverage and sanitizer presets are documented in
[Testing and runtime diagnostics](testing.md).

## Building on Windows

The Visual Studio generator is multi-configuration. Configuration selects the
architecture and creates one build tree; the build and install commands select
Debug or Release within that tree.

### x64 Debug

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug --parallel
ctest --preset vs2022-x64-debug
cmake --install out/build/vs2022-x64 --config Debug
```

### x64 Release

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-release --parallel
ctest --preset vs2022-x64-release
cmake --install out/build/vs2022-x64 --config Release
```

Use `vs2022-win32`, `vs2022-win32-debug`, and `vs2022-win32-release` for the
equivalent 32-bit builds.

The default installation prefixes are:

```text
out/install/vs2022-x64
out/install/vs2022-win32
```

Debug libraries have a `d` filename postfix. Debug and Release may therefore
be installed into the same architecture-specific prefix without overwriting
one another.

## Building on Linux

The Ninja presets are single-configuration: each Debug or Release preset has a
separate build and installation directory.

### GCC x64 Debug

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --parallel
ctest --preset linux-gcc-debug
cmake --install out/build/linux-gcc-debug
```

### Clang x64 Release

```bash
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release --parallel
ctest --preset linux-clang-release
cmake --install out/build/linux-clang-release
```

Other combinations follow the same naming pattern:

```text
linux-gcc-debug
linux-gcc-release
linux-gcc-x86-debug
linux-gcc-x86-release
linux-clang-debug
linux-clang-release
linux-clang-x86-debug
linux-clang-x86-release
```

Each preset installs to `out/install/<preset-name>`.

## Configuring without a project preset

Presets are convenience configurations. A custom build may invoke CMake
directly, provided all required packages are discoverable.

For example, a GCC build using vcpkg can be configured with:

```bash
cmake -S . -B out/build/custom-gcc -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_INSTALL_PREFIX="$PWD/out/install/custom-gcc" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DVCPKG_HOST_TRIPLET=x64-linux \
    -DBUILD_TESTING=ON

cmake --build out/build/custom-gcc --parallel
ctest --test-dir out/build/custom-gcc --verbose
cmake --install out/build/custom-gcc
```

Choose the compiler, generator, architecture, toolchain file, and ABI-related
options before the first configuration of a build directory. If one of these
fundamental settings changes, use a new build directory rather than reusing a
cache created for another toolchain.

## Customizing a preset

Command-line cache definitions supplied after `--preset` override values from
that preset. For example, this configures the recommended GCC build with C++20,
without tests, and with a custom installation prefix:

```bash
cmake --preset linux-gcc-debug \
    -DCMAKE_CXX_STANDARD=20 \
    -DBUILD_TESTING=OFF \
    -DCMAKE_INSTALL_PREFIX="$PWD/out/install/custom"
```

The libraries require at least C++17 through target compile features. Setting
`CMAKE_CXX_STANDARD` to 20 or a newer supported standard is allowed; setting it
below 17 does not lower the target requirement.

For repeatable personal or downstream configurations, prefer a separate user
preset that inherits one of the project presets over a long command line. Do
not edit the checked-in presets merely to describe a machine-local path.

## MSVC runtime selection

The Windows presets use the static multithreaded runtime:

```text
Debug:   /MTd
Release: /MT
```

This is selected through `CMAKE_MSVC_RUNTIME_LIBRARY`. A custom build may use
the DLL runtime instead, but the application and all linked libraries must use
compatible runtime and linkage settings. The vcpkg target triplet must be
selected or customized accordingly; changing only the compiler flag while
retaining dependencies built for another CRT model can produce ABI conflicts
or linker failures.

For a multi-configuration generator, the dynamic runtime expression is:

```text
MultiThreaded$<$<CONFIG:Debug>:Debug>DLL
```

Treat compiler version, target architecture, runtime model, and dependency
triplet as one coherent toolchain configuration.

## Using dependencies without vcpkg

The source tree does not call the vcpkg executable directly. It uses standard
`find_package()` calls and imported CMake targets, so vcpkg can be replaced by
another dependency provider or by prebuilt packages.

In such a configuration, omit the vcpkg toolchain and make compatible package
configurations discoverable through `CMAKE_PREFIX_PATH`, package-specific
`<PackageName>_DIR` variables, or another CMake toolchain:

```bash
cmake -S . -B out/build/custom -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/boost;/opt/protobuf" \
    -DBUILD_TESTING=OFF
```

The dependency provider must expose the targets expected by the project and
must use a compatible architecture and ABI. When tests are enabled, it must
also provide the GoogleTest CMake package.

## Installation layout

Given an installation prefix `<prefix>`, CMake installs a conventional layout:

```text
<prefix>/
|-- include/
|   |-- common-lib/
|   |-- rpc-lib/
|   `-- win-lib/                 Windows installations only
`-- lib/
    |-- <static libraries>
    `-- cmake/
        `-- vshalygin-common/
            |-- vshalygin-common-config.cmake
            |-- vshalygin-common-config-version.cmake
            `-- vshalygin-common-targets*.cmake
```

The exact library filename follows platform conventions. Because the current
targets are static, no project DLL or shared object is installed under `bin/`.

To install somewhere other than the preset default, override
`CMAKE_INSTALL_PREFIX` while configuring. `cmake --install --prefix` may also
replace the prefix for one install invocation:

```bash
cmake --install out/build/linux-gcc-release --prefix /opt/vshalygin-common
```

On a multi-configuration generator, retain the configuration argument:

```powershell
cmake --install out/build/vs2022-x64 --config Release `
    --prefix C:/local/vshalygin-common
```

## Consuming the installed package

A CMake consumer finds the package configuration and links the required
imported target:

```cmake
cmake_minimum_required(VERSION 3.25)

project(my_application LANGUAGES CXX)

find_package(vshalygin-common CONFIG REQUIRED)

add_executable(my-application main.cpp)
target_link_libraries(my-application PRIVATE vshalygin::rpc-lib)
```

Available imported targets are:

- `vshalygin::common-lib`;
- `vshalygin::rpc-lib`;
- `vshalygin::win-lib` on Windows.

Point CMake at the installation prefix during consumer configuration:

```bash
cmake -S path/to/application -B path/to/build \
    -DCMAKE_PREFIX_PATH=/path/to/vshalygin-common
```

`CMAKE_PREFIX_PATH` is a semicolon-separated CMake list and may contain more
than one prefix. Alternatively, set `vshalygin-common_DIR` directly to the
directory containing `vshalygin-common-config.cmake`.

The package configuration finds its own public dependencies and then defines
the imported targets. Include directories, compile requirements, and
transitive link dependencies propagate through those targets; consumers
should not hard-code paths to installed headers or library files.

## Validating the installation with `rpc-example`

Install the libraries first, then configure the example from its own project
directory with the matching preset:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --parallel
cmake --install out/build/linux-gcc-debug

cd example/rpc-example
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --parallel
```

The example preset points `CMAKE_PREFIX_PATH` at the corresponding default
installation directory. If the package was installed elsewhere, override that
variable during example configuration.

Do not mix package and example configurations with incompatible compilers,
architectures, standard-library ABIs, or MSVC runtime models.

## Reconfiguring and troubleshooting

- Confirm that `VCPKG_ROOT` is set in the environment visible to CMake or the
  IDE, not only in a different terminal session.
- Install Ninja before selecting a provided Linux preset.
- Use a fresh build directory after changing the compiler, generator,
  architecture, toolchain, or vcpkg triplet.
- Ensure the package architecture and configuration match the consumer.
- If `find_package(vshalygin-common)` fails, verify that
  `CMAKE_PREFIX_PATH` names the installation prefix, or set
  `vshalygin-common_DIR` to its `lib/cmake/vshalygin-common` directory.
- If a transitive package is not found, make Boost and Protobuf discoverable in
  the consumer configuration as well. Static imported targets still require
  their link dependencies to be available when the final executable is linked.

## Related documentation

- [Repository overview](../README.md)
- [Architecture](architecture.md)
- [Testing and runtime diagnostics](testing.md)
- [RPC example](../example/rpc-example/README.md)
