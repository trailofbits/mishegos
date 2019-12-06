Adding a mishegos worker
========================

Adding a new worker to mishegos is (relatively) straightforward.

This page makes an attempt to document the process, but no guarantees about
correctness or being up-to-date are made. When in doubt refer to
a simple worker already in the tree, like
[capstone](https://github.com/trailofbits/mishegos/tree/master/src/worker/capstone).

## Adding the worker

A good worker is self contained within its `./src/worker/WORKERNAME/` directory.

That directory should look something like this:

```
./src/worker/WORKERNAME/:
    SOME_SUBMODULE/
    Makefile
    WORKERNAME.c
```

Each member is discussed below.

### `SOME_SUBMODULE/`

If your worker requires a disassembly library that is **either** (1) actively maintained **or**
(2) is unavailable in popular package managers, then it should be submoduled within the worker
directory. Multiple submodules (or recursive submodules, if necessary) are fine; see the XED worker
for an example.

### `Makefile`

Your worker directory should include a single `Makefile` that builds both the target disassembler
and the mishegos worker.

Two `make` targets are required:

* `all`: Build all dependencies and the worker's shared object
* `clean`: Clean the worker's shared object and, *optionally*, the builds of all dependencies

Your `all` target should produce some reasonably named shared object (`WORKERNAME.so` is
currently common in the codebase) in the worker directory. You'll need this shared object's path
later.

### `WORKERNAME.c`

`WORKERNAME.c` should implement the mishegos worker ABI, which is the following:

```c
char *worker_name;
void worker_ctor();
void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length);
void worker_dtor();
```

See the existing workers and header files for type and usage examples.

`worker_name` is a static string that *uniquely identifies the worker*. Duplicating `worker_name`
across different kinds of workers will cause very bad things to happen.

`worker_ctor` and `worker_dtor` are **optional** and run on worker process startup and termination,
respectively.

## Integrating into the build

Once you have a worker in place, you'll have to modify a few files to get mishegos to build
and fuzz with it.

### `./src/workers/Makefile`

This `Makefile` contains a `WORKERS` variable. Add `WORKERNAME` (or whatever you named
your worker directory) to it.

### `./Makefile`

The top-level `Makefile` contains an `ALL_SRCS` variable. This variable has a `find` expression
in it that excludes submodule sources from automated linting tasks. Add glob(s) matching your
worker's submodule(s) to it.

### `./workers.spec`

This is a newline-delimited list of shared objects that `mishegos` (the main fuzzer binary)
takes via an argument. Add the path to your worker shared object to it.
