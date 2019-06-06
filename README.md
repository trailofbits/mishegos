mishegos
========

A differential fuzzer for x86 decoders.

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

```
./src/mishegos/mishegos ./workers.spec
```

### Performance notes

All numbers below correspond to the following run:

```
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
    * Avoid `longjmp`/`setjmp` for error recovery within worker processes
    * Maybe use a better data structure for input/output/cohort slots
* Add more workers:
    * https://github.com/vmt/udis86
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
    * Really basic: an HTML table, with color-coded cells
    * Basic but not really basic: some kind of mouse-over differential visualization
