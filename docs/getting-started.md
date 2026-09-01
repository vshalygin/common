# Getting started

This tutorial takes a new library user or contributor from an empty development
environment to a tested and installed Debug build, followed by a working
`rpc-example`. Choose either the Windows or Linux path and run commands in the
order shown.

For alternative compilers, x86 builds, Release builds, custom dependency
providers, and installation options, see
[Building and installing](build-and-install.md).

## Windows: MSVC x64 Debug

### 1. Install the tools

Install:

- Git;
- CMake 3.25 or newer;
- Visual Studio 2022 with the **Desktop development with C++** workload.

The checked-in Windows preset uses the v143 toolset and builds for x64. Open a
new PowerShell session and verify the tools that are expected on `PATH`:

```powershell
git --version
cmake --version
```

### 2. Clone the project and vcpkg

Choose any development directory. The following example keeps vcpkg next to
the project:

```powershell
New-Item -ItemType Directory -Force C:/Coding | Out-Null
Set-Location C:/Coding

git clone https://github.com/vshalygin/common.git
git clone https://github.com/microsoft/vcpkg.git
```

Bootstrap vcpkg and expose its root to the current shell:

```powershell
C:/Coding/vcpkg/bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = "C:/Coding/vcpkg"
```

`VCPKG_ROOT` must be set in every new shell that runs a project preset. It may
instead be defined persistently in the user environment or in the environment
used to launch the IDE.

### 3. Configure the libraries and tests

Enter the repository root and configure the recommended x64 build:

```powershell
Set-Location C:/Coding/common
cmake --preset vs2022-x64
```

On the first configuration, vcpkg resolves the manifest and builds dependencies
that are not already available in its binary cache. This can take considerably
longer than later configurations.

Configuration succeeds when CMake finishes generating the Visual Studio build
tree under `out/build/vs2022-x64`.

### 4. Build and run the tests

```powershell
cmake --build --preset vs2022-x64-debug --parallel
ctest --preset vs2022-x64-debug
```

CTest prints the output of every discovered GoogleTest case. A successful run
ends with all tests passing.

### 5. Install the package

```powershell
cmake --install out/build/vs2022-x64 --config Debug
```

The default installation prefix is:

```text
C:/Coding/common/out/install/vs2022-x64
```

It now contains the static Debug libraries, headers, and CMake package metadata
used by external consumers.

### 6. Build and run `rpc-example`

The example is a separate CMake project. Its matching preset already points to
the installation created in the previous step:

```powershell
Set-Location C:/Coding/common/example/rpc-example
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug --parallel
```

Run the executable from the repository root output tree:

```powershell
& C:/Coding/common/out/build/rpc-example/vs2022-x64/Debug/rpc-example.exe
```

Select `0` for the in-memory transport. Enter `help` to list the available
commands, try `add client`, and enter `exit` to stop the application.

The completed example build confirms that another CMake project can discover
the installed package and use `rpc-lib` through its exported target.

## Linux: GCC x64 Debug

The commands below target Ubuntu 22.04 or a compatible Debian-based
distribution. Package names may differ on other distributions.

### 1. Install the tools

Install the compiler, Git, Python, and the utilities needed by vcpkg:

```bash
sudo apt-get update
sudo apt-get install --yes \
    build-essential \
    ca-certificates \
    curl \
    git \
    pkg-config \
    python3 \
    python3-pip \
    tar \
    unzip \
    zip
```

Ubuntu 22.04 provides older CMake and Ninja versions than the recommended
project setup. Install the same versions used by the Linux toolchain images in
the user-local Python binary directory:

```bash
python3 -m pip install --user \
    "cmake==4.4.2" \
    "ninja==1.11.1.1"
export PATH="$HOME/.local/bin:$PATH"
```

Verify the selected tools:

```bash
g++ --version
cmake --version
ninja --version
```

### 2. Clone the project and vcpkg

```bash
mkdir -p "$HOME/Coding"
cd "$HOME/Coding"

git clone https://github.com/vshalygin/common.git
git clone https://github.com/microsoft/vcpkg.git
```

Bootstrap vcpkg and configure the current shell:

```bash
"$HOME/Coding/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/Coding/vcpkg"
```

Add the `VCPKG_ROOT` and `PATH` exports to the appropriate shell profile if they
should persist across terminal sessions.

### 3. Configure the libraries and tests

```bash
cd "$HOME/Coding/common"
cmake --preset linux-gcc-debug
```

The first configuration may be slow because vcpkg builds dependencies that are
not present in its binary cache. CMake writes the generated Ninja build tree to
`out/build/linux-gcc-debug`.

### 4. Build and run the tests

```bash
cmake --build --preset linux-gcc-debug --parallel
ctest --preset linux-gcc-debug
```

CTest prints the output of every discovered GoogleTest case. A successful run
ends with all tests passing.

### 5. Install the package

```bash
cmake --install out/build/linux-gcc-debug
```

The default installation prefix is:

```text
$HOME/Coding/common/out/install/linux-gcc-debug
```

It contains the static Debug libraries, headers, and CMake package metadata
used by external consumers.

### 6. Build and run `rpc-example`

```bash
cd "$HOME/Coding/common/example/rpc-example"
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --parallel
```

Run the executable:

```bash
"$HOME/Coding/common/out/build/rpc-example/linux-gcc-debug/rpc-example"
```

Select `0` for the in-memory transport. Enter `help` to list the available
commands, try `add client`, and enter `exit` to stop the application.

The completed example build confirms that another CMake project can discover
the installed package and use `rpc-lib` through its exported target.

## What was created

After completing either path, the repository contains three distinct kinds of
output:

```text
out/
|-- build/<library-preset>/       Libraries and tests build tree
|-- install/<library-preset>/     Installed CMake package
`-- build/rpc-example/<preset>/   Standalone example build tree
```

The source directories remain free of generated Protobuf files and compiler
outputs.

## Next steps

- Read [Building and installing](build-and-install.md) for the complete preset
  matrix and custom configurations.
- Read [Testing](testing.md) for coverage and sanitizer
  workflows.
- Read [Architecture](architecture.md) for project boundaries and dependency
  direction.
- Read the [`rpc-example` guide](../example/rpc-example/README.md) for the full
  interactive command set.
