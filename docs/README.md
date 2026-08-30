# Documentation

This directory contains the project-wide guides and architectural
documentation for `common`. Use this page to choose the shortest path to the
information you need; the individual documents remain the source of detail.

## Choose a path

### I want to build and use the libraries

1. Follow [Getting started](getting-started.md) for a complete first build on
   Windows or Linux.
2. Read [Building and installing](build-and-install.md) when you need another
   compiler, architecture, build type, dependency source, installation prefix,
   or MSVC runtime.
3. Continue with the overview for the library you intend to use:
   [`common-lib`](../common-lib/README.md),
   [`rpc-lib`](../rpc-lib/README.md), or
   [`win-lib`](../win-lib/README.md).

### I want to understand the design

1. Start with the repository-level [Architecture](architecture.md).
2. For asynchronous composition, value access, chaining, and lifetime rules,
   continue with [Future semantics](future-semantics.md).
3. For RPC concepts, component responsibilities, protocol flow, ownership,
   and extension boundaries, continue with
   [RPC architecture and protocol](rpc-architecture.md).

### I want to work on the project

1. Read [Architecture](architecture.md) to understand component and dependency
   boundaries.
2. Use [Building and installing](build-and-install.md) to select or customize a
   development configuration.
3. Read [Testing](testing.md) for the project test organization, local test and
   diagnostic presets, coverage output, and CI execution model.
4. Consult the [roadmap](../ROADMAP.md) for planned work and project direction.

## Guides

| Document | Purpose |
| --- | --- |
| [Getting started](getting-started.md) | A first successful configure, build, test, install, and standalone example run on Windows or Linux. |
| [Building and installing](build-and-install.md) | The complete build model, presets, customization points, dependency integration, installation layout, and package consumption. |
| [Testing](testing.md) | Project test suites, supported ways to run them, coverage and sanitizer configurations, and CI test execution. |

## Architecture

| Document | Purpose |
| --- | --- |
| [Architecture](architecture.md) | Repository boundaries, libraries, dependency direction, asynchronous execution model, extensibility, and packaging model. |
| [Future semantics](future-semantics.md) | The asynchronous result model, chaining, error propagation, value locking, flattening, lifetime rules, and usage examples. |
| [RPC architecture and protocol](rpc-architecture.md) | RPC layers, connection establishment, authentication, request processing, wire protocol, transport contract, ownership, and lifetime behavior. |

## Component documentation

| Component | Documentation |
| --- | --- |
| `common-lib` | [Library overview](../common-lib/README.md) |
| `rpc-lib` | [Library overview](../rpc-lib/README.md) |
| `win-lib` | [Library overview](../win-lib/README.md) |
| Examples | [Standalone RPC example](../example/rpc-example/README.md) |

## Project direction

See [ROADMAP.md](../ROADMAP.md) for planned capabilities and larger areas of
future work. The roadmap communicates direction rather than a compatibility or
delivery commitment.

For a concise repository overview, supported platforms, toolchain matrix, and
CI summary, return to the [root README](../README.md).
