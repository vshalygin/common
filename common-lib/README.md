# common-lib

`common-lib` is the portable utility foundation of the `common` repository. It
provides reusable C++ building blocks that are independent of RPC and
operating-system-specific APIs. Applications may use it directly, while the
other libraries in this repository build higher-level functionality on top of
it.

The library supports Windows and Linux, requires C++17 or newer, and is
distributed as the installed CMake target `vshalygin::common-lib`. Its public
C++ API is in the `vshalygin::cl` namespace.

## Scope

Functionality belongs in `common-lib` when it is broadly reusable and can be
expressed without depending on a particular application domain, RPC protocol,
or operating-system API. The library concentrates common infrastructure in one
place so that applications and higher-level components can share the same
ownership, execution, synchronization, and data-handling conventions.

Platform-specific facilities and remote procedure call behavior are outside
the scope of `common-lib`.

## Capabilities

### Asynchronous execution

The library provides an execution model for scheduling work and composing
asynchronous operations. It includes thread-pool execution, serialized
execution contexts, tasks, promises, futures, continuation chains, and timers.
Execution resources are supplied explicitly rather than created invisibly by
each asynchronous object, allowing an application to control concurrency and
shutdown at the system boundary.

Future values remain protected while shared between asynchronous operations.
Access is explicit, making synchronization visible at the point where a value
is inspected or modified. Operations built on this model may extend internal
state through asynchronous chains, but externally supplied non-owning objects
must still satisfy their documented lifetime requirements.

See [Future semantics](../docs/future-semantics.md) for the
complete model, chaining and flattening behavior, code examples, guarantees,
and lifetime rules.

### Synchronization

Synchronization facilities cover notification, guarded value access,
lightweight locking, and coordinated acquisition of multiple ordered locks.
They are intended to make concurrent access policies explicit and to support
components that must maintain invariants across asynchronous callbacks.

Synchronization governs access to an object; it does not by itself own that
object or extend its lifetime. Code using non-owning references must therefore
provide a separate lifetime guarantee.

### Memory and ownership

Memory utilities support explicit ownership and allocation policies, including
unique ownership with configurable representation and helpers for objects
whose shared ownership is established separately from construction. These
facilities are designed for cases where standard ownership types provide the
right model but an application needs additional control over storage or object
layout.

### Data and callable utilities

The library contains reusable value, buffer, view, callable, tuple, and type
adaptation facilities. Owning and non-owning representations are separated so
that data movement and lifetime assumptions remain visible in interfaces.

### Compile-time programming

Compile-time traits and transformations support generic code that operates on
functions, tuples, and qualified types. They provide the metaprogramming
foundation used by template-based facilities elsewhere in the repository and
are also available to consumers with similar requirements.

These categories describe the role of the library rather than an exhaustive
API inventory. The public headers are the authoritative list of available
facilities.

## Using the installed library

Install the repository package, make its prefix visible to CMake, and link the
exported target:

```cmake
find_package(vshalygin-common CONFIG REQUIRED)

add_executable(my-application main.cpp)
target_link_libraries(my-application PRIVATE vshalygin::common-lib)
```

The imported target supplies the include directories, compile requirements,
and transitive link dependencies required by `common-lib`. Consumers should
not reproduce those settings manually.

Public headers use the `common-lib/` include prefix. Facilities may be included
through their category headers:

```cpp
#include <common-lib/memory/memory.h>
#include <common-lib/mpl/mpl.h>
#include <common-lib/synchronization/synchronization.h>
#include <common-lib/thread/thread.h>
#include <common-lib/utils/utils.h>
```

Include only the categories needed by the translation unit. Headers under an
`internal/` directory support public templates and library implementation;
their names and declarations are not public extension points.

## Dependencies

`common-lib` uses the platform threading library and selected Boost facilities.
The repository's default build resolves these dependencies through vcpkg. An
installed package exposes them transitively through its CMake target, so a
consumer links only `vshalygin::common-lib`.

The dependency provider and versions are build choices rather than properties
of the source API. See [Building and installing](../docs/build-and-install.md)
for the provided presets, vcpkg integration, custom toolchains, alternative
dependency discovery, and installation layout.

## Development and validation

The dedicated `common-lib-test` suite validates the library independently of
the other libraries in the project. The repository also provides coverage and
sanitizer configurations for deeper runtime diagnostics. See
[Testing](../docs/testing.md) for the supported local and CI workflows.

## Related documentation

- [Getting started](../docs/getting-started.md)
- [Building and installing](../docs/build-and-install.md)
- [Repository architecture](../docs/architecture.md)
- [Future semantics](../docs/future-semantics.md)
- [Testing](../docs/testing.md)
- [Documentation index](../docs/README.md)
- [Project roadmap](../ROADMAP.md)
