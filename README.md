# common

The project installs a CMake config package with these imported targets:

- `vshalygin::common-lib`
- `vshalygin::rpc-lib`
- `vshalygin::win-lib` on Windows

## Build and install

For example, to build and install the Win32 Debug libraries:

```powershell
cmake --preset vs2022-win32
cmake --build --preset vs2022-win32-debug
cmake --install out/build/vs2022-win32 --config Debug
```

The recommended preset installs the package under
`out/install/vs2022-win32`. `CMAKE_INSTALL_PREFIX` can be overridden for a
custom installation location.

## Standalone RPC example

`example/rpc-example` is a separate CMake project and consumes the installed
package through `find_package(vshalygin-common CONFIG REQUIRED)`.

After installing the libraries, configure and build it from its own source
directory:

```powershell
cd example/rpc-example
cmake --preset vs2022-win32
cmake --build --preset vs2022-win32-debug
```

For a custom package location, pass it explicitly:

```powershell
cmake -S example/rpc-example -B out/build/rpc-example-custom `
    -DCMAKE_PREFIX_PATH=C:/path/to/vshalygin-common
```

## Coverage

Line and branch coverage is available locally on Linux with GCC and gcovr
8.6 or newer. Install gcovr, set `VCPKG_ROOT`, and run the coverage workflow:

```bash
python3 -m pip install "gcovr==8.6"
cmake --workflow --preset linux-gcc-coverage
```

The workflow configures and builds the instrumented Debug targets, runs all
portable tests, and writes the reports to
`out/build/linux-gcc-coverage/coverage`. Open `index.html` for the detailed
report. `coverage.xml` contains the same line and branch data in Cobertura
format. `coverage.txt` contains the per-file line report, and
`coverage-summary.md` summarizes line, function, and branch coverage.
