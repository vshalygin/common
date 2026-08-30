# RPC example

`rpc-example` is a small interactive application that demonstrates how to use
`rpc-lib`. It runs one server and lets the user create multiple clients, send
requests in both directions, observe connection identifiers, and remove active
clients.

The example is also an external package-consumption check. It is configured as
an independent CMake project, calls `find_package(vshalygin-common)`, and links
the installed `vshalygin::rpc-lib` target instead of using targets from the
repository source tree.

## Before building

The libraries must already be built and installed with the configuration that
will be used by the example. Follow [Getting started](../../docs/getting-started.md)
for a first package installation or
[Building and installing](../../docs/build-and-install.md) for other
configurations. The matching example presets point `CMAKE_PREFIX_PATH` to the
installation directories produced by the root project presets.

Set `VCPKG_ROOT` to a bootstrapped vcpkg checkout before using the provided
example presets.

## Windows: MSVC x64 Debug

After installing the matching package, configure and build the standalone
example from PowerShell:

```powershell
Set-Location C:/Coding/common/example/rpc-example
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug --parallel
```

Run it:

```powershell
& C:/Coding/common/out/build/rpc-example/vs2022-x64/Debug/rpc-example.exe
```

The `vs2022-win32` configure preset and corresponding Debug or Release build
preset can be used in the same way for a 32-bit build.

## Linux: GCC x64 Debug

After installing the matching package, configure and build the standalone
example:

```bash
cd "$HOME/Coding/common/example/rpc-example"
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --parallel
```

Run it:

```bash
"$HOME/Coding/common/out/build/rpc-example/linux-gcc-debug/rpc-example"
```

Equivalent presets are available for Clang, Release, and x86 configurations.
The package and example presets must use compatible compilers, architectures,
build types, runtime settings, and dependencies.

## Using the application

At startup, select one of the transport options printed by the application.
The in-memory option (`0`) is the simplest way to explore the example without
external configuration.

Enter `help` at any time to print the command list:

| Command | Action |
| --- | --- |
| `help` | Show the available commands. |
| `info` | Show the current clients and server connection count. |
| `add client` | Create a client and connect it to the server. |
| `remove client <client-id>` | Disconnect and remove a client. |
| `send to server <client-id> <message>` | Send a request from one client to the server. |
| `send to client <connection-id> <message>` | Send a request from the server to one connected client. |
| `send to all clients <message>` | Send a request from the server to every connected client. |
| `exit` | Shut down the application. |

Client IDs belong to the example application. Server connection IDs are
assigned by `rpc-lib` and are printed when clients connect. Use the appropriate
identifier for the direction of the command.

A minimal session looks like this:

```text
0
add client
info
send to server 0 hello from client
send to client 0 hello from server
send to all clients broadcast message
remove client 0
exit
```

Use the connection ID reported by the application in place of `0` for
server-to-client commands if a different value was assigned.

## What the example demonstrates

- consuming the installed CMake package from a separate project;
- generating Protocol Buffers sources in the example build tree;
- implementing services on both sides of an RPC connection;
- making asynchronous requests in both directions;
- managing multiple client connections and their lifecycle;
- selecting a transport without changing the service interface.

For the component model, request flow, transport contract, and lifetime rules,
see [RPC architecture and protocol](../../docs/rpc-architecture.md).
