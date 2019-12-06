mishegos
========

[![Build Status](https://travis-ci.com/trailofbits/mishegos.svg?token=DQQpqJG5gna6rypMg4Lk&branch=master)](https://travis-ci.com/trailofbits/mishegos)

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
./src/mishegos/mishegos ./workers.spec
```

`mishegos` checks for three environment variables:

* `V=1` enables verbose output on `stderr`
* `D=1` enables the "dummy" mutation mode for debugging purposes
* `M=1` enables the "manual" mutation mode (i.e., read from `stdin`)

Run an analysis/filter pass group on the results:

```bash
./src/analysis/analysis -p same-size-different-decodings  < /tmp/mishegos > /tmp/mishegos.interesting
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

Within Docker:

* On a Linux server (40 cores, 128GB RAM):
    * 3.5M outputs/minute
    * 5 cores pinned
* On a 2018 Macbook Pro (2+2 cores, 16GB RAM):
    * 300K outputs/minute
    * (All) 4 cores pinned

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
