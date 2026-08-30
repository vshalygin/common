# win-lib

`win-lib` provides reusable C++ components whose purpose or implementation is
specific to Windows. It defines an explicit boundary around direct Windows API
usage so that native resource management and operating-system integration can
be shared without being mixed into otherwise portable application code.

The library is built only for Windows, requires C++17 or newer, and is
distributed as the installed CMake target `vshalygin::win-lib`. Its public C++
API is in the `vshalygin::win` namespace.

## Scope

Functionality belongs in `win-lib` when it is reusable across applications but
depends intrinsically on Windows types, handles, execution facilities, or
system calls. The library may cover any Windows-specific area; it is not tied
to a single application domain or I/O mechanism.

Portable abstractions and application-specific policy are outside its scope.
Keeping this boundary explicit prevents Windows headers, native types, and
platform assumptions from leaking unintentionally into portable targets.

## Design principles

- Native resources use deterministic RAII cleanup.
- Ownership is unique unless Windows or an application explicitly supplies a
  different lifetime model.
- Move operations transfer ownership without duplicating a native handle.
- Native handles remain accessible when an operation cannot be expressed
  usefully without the Windows API.
- Recoverable system failures are represented explicitly rather than hidden.
- Platform details remain contained within the Windows-specific library
  boundary.

## Using the installed library

Install the repository package on Windows, make its prefix visible to CMake,
and link the exported target:

```cmake
find_package(vshalygin-common CONFIG REQUIRED)

add_executable(my-application main.cpp)
target_link_libraries(my-application PRIVATE vshalygin::win-lib)
```

For a consumer that is also configured on other platforms, guard the
Windows-only target explicitly:

```cmake
if(WIN32)
    target_link_libraries(my-application PRIVATE vshalygin::win-lib)
endif()
```

The imported target supplies the include directories and build requirements
associated with the library. Consumers should not reproduce those settings
manually.

Public facilities are available through the `win-lib/` include prefix:

```cpp
#include <win-lib/types/handle.h>
#include <win-lib/types/iocp.h>
```

Headers under an `internal/` directory support the implementation and are not
public extension points. Consumers should use the public handle aliases and
classes rather than depending on their internal traits or templates.

## Requirements and dependencies

`win-lib` requires:

- a Windows target platform;
- a C++17-capable compiler;
- Windows SDK headers and system libraries.

The provided Windows presets use MSVC, but the selected compiler and toolchain
remain build configuration choices. The library does not depend on the other
libraries in this project.

See [Building and installing](../docs/build-and-install.md) for the supported
presets, custom configuration, installation layout, and package discovery.

## Development and validation

The dedicated `win-lib-test` suite validates the library independently of the
other libraries in the project. It is enabled only for Windows builds. See
[Testing](../docs/testing.md) for the supported local and CI workflows.

## Related documentation

- [Getting started](../docs/getting-started.md)
- [Building and installing](../docs/build-and-install.md)
- [Repository architecture](../docs/architecture.md)
- [Testing](../docs/testing.md)
- [Documentation index](../docs/README.md)
- [Project roadmap](../ROADMAP.md)
