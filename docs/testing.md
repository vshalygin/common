# Testing

The test suite verifies the behavior of every library on its supported
platforms and toolchains. Tests are written with GoogleTest and exposed through
CTest. This document describes the test areas and the project-supported ways to
run them.

## Test suites

The repository has one test executable for each library:

| Test suite | Platforms | Purpose |
| --- | --- | --- |
| `common-lib-test` | Linux, Windows | Verifies the portable general-purpose library. |
| `rpc-lib-test` | Linux, Windows | Verifies RPC behavior and the transports available on the target platform. |
| `win-lib-test` | Windows | Verifies Windows-specific components. |

### `common-lib-test`

The suite aims to verify all `common-lib` behavior that can be tested reliably
and deterministically. This includes successful operation, boundary conditions,
failures, and the relevant ownership, lifetime, and concurrency guarantees.

### `rpc-lib-test`

The suite aims to verify all `rpc-lib` behavior that can be tested reliably and
deterministically, from isolated components to complete interactions between
RPC endpoints. Portable behavior runs on both Linux and Windows. Tests for
Windows-specific behavior are included only in Windows builds.

### `win-lib-test`

The suite aims to verify all `win-lib` behavior that can be tested reliably and
deterministically. It is created only for Windows builds.

## Enabling tests

Tests are part of the root build when `BUILD_TESTING=ON`. All recommended
project presets enable this option.

For a custom build, enable it explicitly during configuration:

```bash
cmake -S . -B out/build/custom -DBUILD_TESTING=ON
```

Set `BUILD_TESTING=OFF` when only the libraries are required.

## Running tests locally

Configure and build the project before invoking the matching CTest preset.

For example, on Linux with GCC:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --parallel
ctest --preset linux-gcc-debug
```

On Windows with MSVC:

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug --parallel
ctest --preset vs2022-x64-debug
```

The project provides test presets for every standard build configuration:

| Platform | Test presets |
| --- | --- |
| Windows x64 | `vs2022-x64-debug`, `vs2022-x64-release` |
| Windows Win32 | `vs2022-win32-debug`, `vs2022-win32-release` |
| Linux GCC x64 | `linux-gcc-debug`, `linux-gcc-release` |
| Linux GCC x86 | `linux-gcc-x86-debug`, `linux-gcc-x86-release` |
| Linux Clang x64 | `linux-clang-debug`, `linux-clang-release` |
| Linux Clang x86 | `linux-clang-x86-debug`, `linux-clang-x86-release` |

Each GoogleTest case is visible to CTest independently. CTest names begin with
the owning library:

```text
common-lib.<suite>.<case>
rpc-lib.<suite>.<case>
win-lib.<suite>.<case>
```

The project test presets print complete test output and fail if no tests are
found.

## Coverage

The project provides a GCC x64 coverage workflow for `common-lib` and
`rpc-lib`:

```bash
python3 -m pip install "gcovr==8.6"
cmake --workflow --preset linux-gcc-coverage
```

The workflow builds the instrumented tests, runs them, and generates line,
function, and branch coverage reports under:

```text
out/build/linux-gcc-coverage/coverage/
```

The directory contains a detailed HTML report, a Cobertura XML report, a text
report, and a Markdown summary. The same directory is uploaded as a CI artifact
named `vshalygin-common-linux-gcc-x64-coverage`.

Coverage currently excludes tests, generated sources, external dependencies,
and `win-lib`.

## Sanitizers

The project provides two Clang x64 Linux workflows for additional runtime
validation:

```bash
cmake --workflow --preset linux-clang-asan-debug
cmake --workflow --preset linux-clang-tsan-debug
```

- `linux-clang-asan-debug` runs the portable tests with AddressSanitizer and
  UndefinedBehaviorSanitizer;
- `linux-clang-tsan-debug` runs them with ThreadSanitizer.

The workflows are separate because AddressSanitizer and ThreadSanitizer cannot
be used together in the same process.

## Tests in continuous integration

Tests run in GitHub Actions for:

- every push to `main`;
- every pull request targeting `main`;
- a manual workflow dispatch from any branch.

The standard CI matrix covers:

| Platform | Compilers | Architectures | Configurations |
| --- | --- | --- | --- |
| Linux | GCC, Clang | x64, x86 | Debug, Release |
| Windows | MSVC | x64, Win32 | Debug, Release |

Every matrix entry configures and builds the project and then runs its matching
CTest preset. Windows-only suites and cases are added automatically on Windows.

Separate CI jobs run:

- GCC line, function, and branch coverage;
- AddressSanitizer with UndefinedBehaviorSanitizer;
- ThreadSanitizer.

The CI workflow also installs the libraries and builds `rpc-example` against
the installed package. The example is compiled as an integration check but is
not executed.

## Related documentation

- [Getting started](getting-started.md)
- [Building and installing](build-and-install.md)
- [Architecture](architecture.md)
