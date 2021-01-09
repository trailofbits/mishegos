mishegos
========

[![Build Status](https://img.shields.io/github/workflow/status/trailofbits/mishegos/CI/master)](https://github.com/trailofbits/mishegos/actions?query=workflow%3ACI)

A differential fuzzer for x86 decoders.

![mishegos](https://user-images.githubusercontent.com/3059210/59005797-da89b400-87ec-11e9-8274-321edfa6df45.png)

## Usage

Start with a clone, including submodules:

```bash
git clone --recurse-submodules https://github.com/trailofbits/mishegos
```

### Building

`mishegos` is most easily built within Docker:

```bash
docker build -t mishegos .
```

Alternatively, you can try building it directly.

Make sure you have `binutils-dev` (or however your system provides `libopcodes`) installed:

```bash
make
# or
make debug
```

### Running

Run the fuzzer for a bit:

```bash
./src/mishegos/mishegos ./workers.spec > /tmp/mishegos
```

`mishegos` checks for three environment variables:

* `V=1` enables verbose output on `stderr`
* `D=1` enables the "dummy" mutation mode for debugging purposes
* `M=1` enables the "manual" mutation mode (i.e., read from `stdin`)
* `MODE=mode` can be used to configure the mutation mode in the absence of `D` and `M`
    * Valid mutation modes are `sliding` (default), `havoc`, and `structured`

Convert mishegos's raw output into JSONL suitable for analysis:

```bash
./src/mish2jsonl/mish2jsonl /tmp/mishegos > /tmp/mishegos.jsonl
```

`mish2jsonl` checks for `V=1` to enable verbose output on `stderr`.

Run an analysis/filter pass group on the results:

```bash
./src/analysis/analysis -p same-size-different-decodings < /tmp/mishegos.jsonl > /tmp/mishegos.interesting
```

Generate an ~ugly~ pretty visualization of the filtered results:

```bash
./src/mishmat/mishmat < /tmp/mishegos.interesting > /tmp/mishegos.html
open /tmp/mishegos.html
```

### Contributing

We welcome contributors to mishegos!

A guide for adding new disassembler workers can be found [here](./docs/adding_a_worker.md).

### Performance notes

All numbers below correspond to the following run:

```bash
V=1 timeout 60s ./src/mishegos/mishegos ./workers.spec > /tmp/mishegos
```

Outside Docker:

* On a Linux desktop (Ubuntu 20.04, Ryzen 5 3600, 32GB DDR4):
    * Commit [`d80063a`](https://github.com/trailofbits/mishegos/commit/d80063a575c4b10d5f787ac88f45d44c8e7f9937)
    * 8 workers (no `udis86`) + 1 `mishegos` fuzzer process
    * 8.7M outputs/minute
    * 9 cores pinned

## TODO

* Performance improvements
    * Break cohort collection out into a separate process (requires re-addition of semaphores)
    * Maybe use a better data structure for input/output/cohort slots
* Add a scaling factor for workers, e.g. spawn `N` of each worker
* Pre-analysis normalization (whitespace, immediate representation, prefixes)
* Analysis strategies:
    * Filter by length, decode status discrepancies
    * Easy: lexical comparison
    * Easy: reassembly + effects modeling (maybe with microx?)
* Scoring ideas:
    * Low value: Flag/prefix discrepancies
    * Medium value: Decode success/failure/crash discrepancies
    * High value: Decode discrepancies with differing control flow, operands, maybe some immediates
* Visualization ideas:
    * Basic but not really basic: some kind of mouse-over differential visualization
