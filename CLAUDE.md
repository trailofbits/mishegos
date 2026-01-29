# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Mishegos is a differential fuzzer for x86 decoders developed by Trail of Bits. It discovers bugs in x86 instruction decoders by comparing outputs from multiple disassembler implementations in parallel.

## Build Commands

```bash
make              # Build all: mishegos, workers, mish2jsonl
make debug        # Debug build with symbols (-g flag)
make worker       # Build worker modules only
make fmt          # Format code with clang-format
make lint         # Static analysis with cppcheck
make clean        # Clean build artifacts

# Build specific workers only
WORKERS="bfd capstone" make worker
```

**Docker development (recommended for fast iteration):**
```bash
# Build the deps image once (includes all build tools, ~1.8GB)
docker build --target deps -t mishegos:deps .

# Mount source and build - supports incremental builds
docker run --rm -v $(pwd):/app/mishegos mishegos:deps make clean all -j $(nproc)

# Run smoketest with mounted source
docker run --rm -v $(pwd):/app/mishegos mishegos:deps bash -c \
  'echo -n "90" | ./src/mishegos/mishegos -m manual ./workers.spec | ./src/mish2jsonl/mish2jsonl'

# Run fuzz test with mounted source
docker run --rm -v $(pwd):/app/mishegos mishegos:deps bash -c \
  'timeout 5s ./src/mishegos/mishegos ./workers.spec > /tmp/out.bin; \
   ./src/mish2jsonl/mish2jsonl /tmp/out.bin | head -5'
```

**Docker production build:**
```bash
# Build minimal deploy image (~270MB, no build tools)
docker build -t mishegos .

# Run fuzz test
mkdir -p ./workspace
docker run --rm -v ./workspace:/workspace mishegos bash -c \
  'timeout 5s ./src/mishegos/mishegos ./workers.spec > /workspace/out.bin; \
   ./src/mish2jsonl/mish2jsonl /workspace/out.bin > /workspace/out.jsonl'
```

The Dockerfile uses three stages: `deps` (build tools), `build` (compilation), `deploy` (minimal runtime).

## Running the Fuzzer

```bash
./src/mishegos/mishegos ./workers.spec > /tmp/mishegos
V=1 ./src/mishegos/mishegos ./workers.spec    # Verbose mode
MODE=havoc ./src/mishegos/mishegos ./workers.spec  # Havoc mutation mode

# Manual mode - decode specific hex input (useful for testing)
echo -n "90" | ./src/mishegos/mishegos -m manual ./workers.spec | ./src/mish2jsonl/mish2jsonl
```

Mutation modes: `sliding` (default), `havoc`, `structured`

## Architecture

### Data Flow Pipeline

```
Input Generation (mutator.c)
    ↓
Fuzzer (mishegos.c) - Futex-based threading & synchronization
    ↓ (loads .so workers dynamically)
11 Disassemblers process in parallel
    ↓
Binary cohort output (streaming)
    ↓
mish2jsonl - Binary to JSONL conversion
    ↓
analysis - Ruby passes (filters, deduplication, etc.)
    ↓
mishmat - HTML visualization
```

### Core Components

| Directory | Purpose |
|-----------|---------|
| `src/mishegos/` | Main fuzzer engine - mishegos.c (threading, I/O, worker mgmt), mutator.c (input mutation) |
| `src/worker/` | 11 disassembler worker implementations |
| `src/mish2jsonl/` | Binary output to JSONL converter |
| `src/mishmat/` | HTML visualization generator (Ruby) |
| `src/analysis/pass/` | 19 Ruby analysis passes (filters, transforms) |
| `src/include/mish_common.h` | Core data structures: input_slot, output_slot, decode_status |

### Worker Architecture

Each worker is a shared library implementing this ABI (`src/worker/worker.h`):

```c
char *worker_name;                                          // Unique identifier
void worker_ctor(void);                                     // Optional initialization
void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length);
void worker_dtor(void);                                     // Optional cleanup
```

**Workers:** bfd, capstone, dynamorio, fadec, xed, zydis, bddisasm, iced (Rust), ghidra, yaxpeax-x86 (Rust), llvm

### Key Constants (mish_common.h)

- Max instruction: 15 bytes (x86-64)
- Max decoded output: 248 bytes
- Max workers: 31
- Slots per chunk: 4096, Chunks: 16

## Adding a New Worker

1. Create `src/worker/WORKERNAME/` directory
2. Add submodule for disassembler library (if external)
3. Create `Makefile` with `all` and `clean` targets
4. Implement `WORKERNAME.c` with worker ABI
5. Update `src/worker/Makefile` WORKERS list
6. Update root `Makefile` ALL_SRCS to exclude submodules from formatting
7. Add `.so` path to `workers.spec`

See `docs/adding_a_worker.md` for details.

## Code Style

- clang-format with LLVM base style, 100 char line limit, 2-space indentation
- C11 standard (`-std=gnu11`)
- Debug logging: `DLOG()` macro (active when DEBUG flag set)
- Tabs only in Makefiles

## CI

GitHub Actions runs on push/PR and daily:
- **lint**: clang-format check + cppcheck static analysis
- **build**: Ubuntu build with smoketest (NOP decode) and 5-second fuzz test
- **docker-build**: Docker image build with tests
