# rpc-lib

`rpc-lib` provides asynchronous, bidirectional remote procedure calls for C++
applications on Windows and Linux. Protocol Buffers supplies the service and
message model, while transport, authentication, connection lifecycle, and
asynchronous execution remain explicit parts of the library architecture.

The library requires C++17 or newer and is distributed as the installed CMake
target `vshalygin::rpc-lib`. Its public C++ API is in the `vshalygin::rpc`
namespace.

## Purpose and scope

The library is responsible for turning calls to generated service stubs into
asynchronous requests, delivering those requests to a service implemented by a
peer, and returning typed results to the caller. It provides the reusable RPC
infrastructure between application service logic and an underlying
message-oriented transport.

Application policy remains outside the library. Applications choose their
services, messages, authentication rules, transport configuration, retry
strategy, and ownership of system-level execution resources.

## RPC model

An established RPC connection is symmetric. Each peer may call a service
implemented by the other peer while simultaneously processing incoming calls
over the same connection. The terms *client* and *server* describe how the
underlying connection is established; they do not limit the direction in which
RPC requests may travel afterward.

The application-facing endpoint types are parameterized by two Protocol
Buffers types:

- `RemoteStub` represents the service exposed by the peer and is used for
  outgoing calls;
- `LocalService` represents the service implemented locally and handles
  incoming calls.

A client endpoint manages one active peer connection. A server endpoint
accepts and manages multiple peer connections and assigns an identifier to
each one. Connection and lifecycle notifications allow an application to
implement its own higher-level availability and reconnection policy.

## Asynchronous execution

All potentially waiting RPC work is asynchronous. Connection establishment,
authentication, message transfer, response processing, timeouts, and
disconnection completion are expressed through futures and callbacks. No RPC
method blocks the calling thread while waiting for transport activity or
another asynchronous operation to finish.

The only synchronous blocking inside these workflows is the brief acquisition
of mutexes that protect an object's mutable state. Transport I/O and waiting
for asynchronous completion are not performed while such mutexes are held.

The application supplies the execution context used by RPC objects. This keeps
thread count, scheduling, and shutdown under application control rather than
creating hidden worker threads inside each endpoint. The supplied execution
context must remain alive and active until every dependent endpoint, future,
and callback has finished using it.

## Services and protocol

Protocol Buffers generated services define the typed RPC interface. Outgoing
calls use a generated stub; incoming calls are dispatched to an
application-provided service implementation. Request and response messages are
serialized for transfer and correlated so that multiple operations may be in
flight concurrently.

The RPC protocol is independent of the concrete transport. It defines the
information needed to identify a message, select a service method, distinguish
requests from responses and connection-management messages, and report remote
processing results. The complete flow and wire-level contract are documented
in [RPC architecture and protocol](../docs/rpc-architecture.md).

## Transport model

Transport integration is expressed through message-oriented interfaces for an
established pipe endpoint and for creating its client and server sides. Higher
RPC layers depend on those interfaces rather than on operating-system handles,
sockets, or any other concrete communication mechanism.

A transport may use any underlying technology as long as it preserves the
pipe contract, asynchronous completion semantics, cancellation behavior, and
message boundaries required by the library. New transports can therefore be
introduced without changing application services or the public endpoint
model.

## Authentication and security boundary

Authentication is a request-response exchange performed after the transport
has been established and before the RPC connection is published. An
application-provided authenticator owns the handshake payload and its
validation rules, allowing credentials and policy to evolve independently of
the RPC protocol.

Authentication does not imply confidentiality or transport security. If an
application requires encryption, peer identity at the transport layer, or
protection against message tampering, the selected transport or an additional
security layer must provide it.

## Ownership and lifetime

Asynchronous work may outlive the call that started it. Objects passed with
shared ownership can therefore remain alive until the relevant callback chain
releases them. Objects and execution resources passed through non-owning
references require an external lifetime guarantee from the application.

Destroying an application-facing endpoint starts cancellation and teardown,
but it does not necessarily destroy its internal implementation immediately.
Callbacks already executing or queued on another thread may retain that
implementation until their work completes. Application callbacks and service
implementations must also synchronize any state they share across threads.

## Using the installed library

Install the repository package, make its prefix visible to CMake, and link the
exported target:

```cmake
find_package(vshalygin-common CONFIG REQUIRED)

add_executable(my-application main.cpp)
target_link_libraries(my-application PRIVATE vshalygin::rpc-lib)
```

The imported target supplies the include directories, compile requirements,
and transitive library dependencies required by `rpc-lib`. Consumers should
not reproduce those settings manually.

The primary application-facing headers use the `rpc-lib/` include prefix:

```cpp
#include <rpc-lib/client-endpoint.h>
#include <rpc-lib/server-endpoint.h>
#include <rpc-lib/types.h>
```

Authentication and custom transport integration are exposed through their
corresponding public headers under `rpc-lib/authenticator/` and
`rpc-lib/pipe/`. Headers and declarations under `rpc-lib/internal/` explain and
support the implementation but are not public extension points.

## Dependencies

`rpc-lib` depends on Protocol Buffers, the repository's portable execution
infrastructure, platform threading support, and selected Boost facilities. The
default build resolves these dependencies through vcpkg. They are propagated
through the installed CMake target, so a consumer links only
`vshalygin::rpc-lib`.

Dependency versions and providers are build choices rather than properties of
the RPC source API. See
[Building and installing](../docs/build-and-install.md) for the provided
presets, vcpkg integration, custom toolchains, alternative dependency
discovery, installation layout, and standalone package validation.

## Development and validation

The dedicated `rpc-lib-test` suite validates RPC behavior across the supported
platforms and configurations. The repository also provides coverage and
sanitizer configurations for deeper runtime diagnostics. See
[Testing](../docs/testing.md) for the supported local and CI workflows.

## Related documentation

- [RPC architecture and protocol](../docs/rpc-architecture.md)
- [Getting started](../docs/getting-started.md)
- [Building and installing](../docs/build-and-install.md)
- [Repository architecture](../docs/architecture.md)
- [Testing](../docs/testing.md)
- [Documentation index](../docs/README.md)
- [Standalone RPC example](../example/rpc-example/README.md)
- [Project roadmap](../ROADMAP.md)
