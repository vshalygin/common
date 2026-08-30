# Roadmap

This roadmap describes planned work. Priorities and implementation order may
change as the project evolves.

## Planned work

- Add a Linux transport based on Unix domain sockets.
- Add randomized stress testing to exercise concurrent and asynchronous
  behavior under varying execution orders and workloads.
- Add fuzzing for message parsing and protocol processing.
- Increase branch coverage to 80%.
- Profile RPC throughput and latency.
- Add documentation comments to public headers, covering API contracts,
  thread-safety, ownership, and lifetime requirements.
- Define and implement deterministic termination behavior for unrecoverable
  failures.
- Introduce public API versioning and a compatibility policy.
- Add Windows XP support.
- Provide a production authenticator, with transport-level traffic encryption
  and integrity verification.
- Resolve existing tracked issues and newly discovered defects.
