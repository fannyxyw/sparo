# sparo

A C++ foundational library for system-level development.

## Supported Platforms

| OS | Architecture | Compiler | Version | Standard Library | Status |
|----|-------------|----------|-------|-----------------|--------|
| Ubuntu 24.04 | ARM64 (AArch64) | clang++ | ≥ 21.0 | libstdc++ / libc++ | Verified |

## Build

### Build System

The project uses **GN** for build file generation and **Ninja** for compilation.
The top-level `Makefile` wraps these tools for convenience.

- GN generates `build.ninja` files under `src/out/debug` or `src/out/release`.
- Ninja drives the actual compilation with up to 16 parallel jobs (`-j16`).
- `bear` is used alongside Ninja to produce `compile_commands.json` for tooling.

### Toolchain / Compiler

Two toolchains are defined in `src/build/toolchain/BUILD.gn`:

| Toolchain | Compiler | Linker | GN arg |
|-----------|----------|--------|--------|
| `clang` (default) | `clang` / `clang++` | `clang++` | *(default)* |

The default toolchain is `clang` (set by `set_default_toolchain("//build/toolchain:clang")`).
To switch to GCC, pass the toolchain override via GN args or modify `BUILDCONFIG.gn`.

**Clang version requirement**: Clang **21.0 or higher** is required for full C++17 support and compatibility with the project's codebase.

### Standard Library

| Standard Library | Compiler flag | When to use |
|-----------------|---------------|-------------|
| **libstdc++** (default) | *(none)* | GCC / system default |
| **libc++** | `-stdlib=libc++` | Clang + LLVM runtime |

Use `libcxx=y` to enable libc++:

```sh
make libcxx=y          # clang++ with -stdlib=libc++
```

### Prerequisites

Install required dependencies first:

```sh
bash build/install-deps.sh
```

Ensure the following tools are available on your `PATH`:
- `gn` — build file generator
- `ninja` — build executor
- `bear` — compile commands extractor
- `clang` / `clang++` (default) or `gcc` / `g++`

### Build Commands

```sh
# Default build (clang, C++17, debug, libstdc++)
make

# Release build (optimized, -O2)
make debug=n

# Debug build (symbols, -g -O0)
make debug=y

# Use libc++ (clang + LLVM libc++)
make libcxx=y

# Release + libc++
make debug=n libcxx=y

# Strip output binary
make strip=y

# Clean all build artifacts
make clean

# Copy output without rebuilding
make install
```

Build output is placed in `out/` (copied from `src/out/debug` or `src/out/release`).

### Build Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `debug` | `y` / `n` | `y` | `y`: debug symbols + `-O0`; `n`: optimized `-O2` |
| `libcxx` | `y` / `n` | `n` | Use LLVM libc++ (`-stdlib=libc++`) instead of libstdc++ |
| `strip` | `y` / `n` | `n` | Strip debug symbols from the output binary |

Default C++ standard: **C++17**.

## Modules

### base/ — Core Components

- **ThreadPool** (thread_pool.h)
  A multi-threaded task pool supporting task submission, waiting for all tasks to complete, active task counting, and completion callbacks.

- **TaskLoop** (task/task_loop.h)
  A single-threaded task loop for serializing asynchronous task scheduling.

- **IntrusiveHeap** (intrusive_heap.h)
  An intrusive min-heap with HeapHandle support for tracking element positions, enabling efficient in-place updates and removals.

- **mpmc_bounded_queue** (bounded-mpmc-queue.h)
  A bounded lock-free multi-producer multi-consumer (MPMC) queue designed for high-concurrency scenarios.

- **CommandLine** (command_line.h/cc)
  Command-line argument parser supporting registration and lookup of switches and parameter values.

- **containers/**
  - `circular_deque`: Circular double-ended queue.
  - `flat_map` / `flat_set`: Sorted-array-based flat map/set with cache-friendly layout.
  - `flat_tree`: Underlying sorted tree implementation for flat containers.

- **strings/**
  - `StringPiece` / `StringView`: Zero-copy string views compatible with std::string_view.

- **numerics/**
  Utilities for safe numeric operations (overflow detection, etc.).

- **threading/**
  Thin wrappers around fundamental threading primitives.

### mmap/ — Memory Mapping


Examples and utilities for shared memory and mmap, exploring inter-process data sharing via memory-mapped files.

### demo/ — IPC Communication Examples

- **ipc_demo.cc**: Full inter-process communication (IPC) demo.
- **shared_buffer.cc**: Shared-memory-based ring buffer implementation.
- **epoll_demo / epoll_lt_et**: epoll level-triggered and edge-triggered examples.
- **client.cc**: Socket client example.

### test/ — Test Cases

Unit tests and examples covering heap, thread pool, task loop, JSON parsing, socket server, and more.

## TODO
- [ ] lazy: Lazy evaluation / deferred initialization.