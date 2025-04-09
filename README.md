# Linux Port-to-PID Mapping System

## Overview

This project implements a modular Linux-based system that monitors network traffic and maps active ports to the process IDs (PIDs) that use them. Built using a kernel module, user-space daemon, and client applications, it demonstrates:

- Real-time **Netlink communication** between kernel and user space.
- A **multi-component architecture** including a passive kernel sniffer, a daemon, and an analysis tool (`packet_hunter`).
- Custom **thread-safe data structures** and **shared resource management** via smart pointers.
- A **UNIX socket-based IPC mechanism**.
- Modular, scalable C++ design with build automation and VSCode integration.

This was my **first full system-level project**, built from scratch to learn real-world Linux programming, not to optimize for efficiency or deployability.

---

## Project Structure

```
.vscode/                  ← VSCode workspace setup (intelliSense, debugging, tasks)
bash_scripts/             ← Shell utilities for build, cleanup, and menu UI
build/                    ← Output folder for all compiled artifacts
daemon/
  ├── include/            ← Headers for daemon-only modules
  ├── src/                ← Daemon source files
  └── Makefile
kernel_module/            ← Netfilter-based packet sniffer (kernel space)
packet_hunter/
  ├── include/            ← Headers for packet analysis logic
  ├── src/                ← Application logic
  └── Makefile
shared/                   ← Shared code reused across subsystems
  ├── config/             ← Config constants and Netlink protocol definitions
  ├── message_queue/      ← Buffered queue for inter-thread Netlink message passing
  ├── netlink_client/     ← Common Netlink socket logic
  ├── thread_safe_unordered_map/ ← Generic lock-protected hash map template
  └── Makefile
```

---

## Development Timeline & Goals

This project evolved through **trial, error, and exploration**, beginning with simple kernel-to-user communication and expanding into:

- Thread management with `std::thread`, `std::mutex`, `std::shared_mutex`.
- Shared ownership with `std::shared_ptr` for communication queues and maps.
- Real-world daemon features: `syslog`, `SIGTERM`, clean shutdowns.
- Build systems using `Makefile` and VSCode support with `.vscode/` configuration.
- Modular architecture using a `shared/` folder to organize reusable code and protocol definitions.

The result is not an optimized or "shippable" system, but a feature-rich **learning platform**.

---

## Component Details

### Kernel Module (`kernel_module/`)

- Hooks into Netfilter to passively observe TCP/UDP packets.
- Extracts source/destination ports, protocol, and address info.
- Sends metadata to user space using Netlink multicast messages.

### Daemon (`daemon/`)

- Listens on a Netlink socket and pushes incoming messages into a buffered message queue (`MessageQueue`).
- Parses messages and updates a `PortToPidMap` (thread-safe hash map of `port -> pid_t`).
- Accepts client queries via a **UNIX domain socket**.
- Originally used `AppThreadsMap` to manage one thread per client for multithreading practice (later removed).
- Chooses threads over `select()`/`poll()` intentionally to practice fine-grained thread control and mutex usage.
- Fully integrated with `systemd` and uses `syslog` for background logging.

### Packet Hunter (`packet_hunter/`)

- A CLI tool for runtime packet analysis.
- Receives packet metadata from the kernel module (via Netlink).
- For each packet, queries the daemon to resolve which process owns the destination port.
- Stores results in a **map of `pid → packet info`** to associate traffic with processes.
- Supports saving collected data to a file for later analysis.

### Shared Modules (`shared/`)

- **`config/`**: Shared constants and Netlink protocol definitions used by all components.
- **`message_queue/`**: Thread-safe queue buffering Netlink messages before processing.
- **`netlink_client/`**: Common Netlink socket functions for daemon and clients.
- **`thread_safe_unordered_map/`**: Reusable shared-mutex protected hash map template.

---

## Port-to-PID Mapping Strategy

Instead of naively scanning all PIDs and checking their sockets, the daemon uses a more efficient **reverse lookup approach**:

1. Parse `/proc/net/{tcp,udp}` to extract socket inodes and port numbers.
2. Traverse `/proc/[pid]/fd/` and resolve symlinks to find matching inodes.
3. Build a map of `port → pid` on daemon startup, and update incrementally.

This avoids scanning thousands of directories needlessly and reflects realistic conditions for debugging systems.


## Makefiles & Scripts

- Each component has its own modular `Makefile` for independent builds.
- A top-level script (`build_all.sh`) builds all modules into the `build/` directory.
- `clean_all.sh` purges all build artifacts.
- `main_menu.sh` provides a UI to load/unload the kernel module, launch the daemon, or run packet analysis tools for demo or debugging sessions.


## Limitations

### Conceptual Limitations

- **Not production-ready**: Built for learning, not for reliability or performance.
- **Port-to-PID mappings are volatile**:
  - Saving these mapping is maeningless since ports pid mapping are subject to change (saw alot of examples from the daemon logging)
  - Processes may terminate between packet capture and resolution.
  - Short-lived connections are missed entirely.

### Technical Limitations

- **Single-client architecture**: Both the daemon and the kernel module currently support only one active client at a time.
- **Volatile mappings**: The port-to-PID associations are often inaccurate due to:
  - The delay between packet arrival and PID resolution.
  - The short lifespan of many processes and sockets.
- **No async I/O needed**: Since the system supports only a single client at a time, `select()` or `poll()` mechanisms were deemed unnecessary.
- **Accuracy edge cases**: While the daemon does scan and clean its map, short-lived connections or timing mismatches can still lead to missing or incorrect PID resolutions.
- **Deliberately limited reuse**: Almost no code reuse across the project was enforced, as this system was structured primarily as an educational exercise in implementing each component independently.

---

## Future Improvements

- **Decouple `packet_hunter` from the kernel module**: Instead of receiving packets directly from the kernel, refactor it to consume pre-processed data from the daemon only.
- **Stream parsed packets from daemon to client**: Shift the architecture so that the daemon buffers and transmits fully parsed packets (with PID included) to the client, eliminating the need for `packet_hunter` to handle PID resolution.
  - This would require moderate changes to both the client protocol and main logic, but was postponed due to time constraints.

---

## Conclusion

This system was built from the ground up as a **learning journey into Linux systems programming**. It showcases practical experience in:

- Kernel module development
- Netlink communication
- Daemon design and management
- Smart pointer-based shared state management
- Thread-safe concurrency in C++
- Modular project structuring and Makefile usage
- IDE integration and project planning

While not optimized or fully accurate, it represents a **strong foundation** in system-level programming and the confidence to explore more production-oriented designs in the future.
