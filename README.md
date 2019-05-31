mishegos
========

A haphazard x86 decoder fuzzer.

## Usage

Start with a clone, including submodules:

```bash
git clone --recursive-submodules -j$(nproc) https://github.com/trailofbits/mishegos
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

## TODO

* Performance improvements
    * Either break cohort collection out into a separate process or remove cohort semaphores
    * Avoid `longjmp`/`setjmp` for error recovery within worker processes
    * Maybe use a better data structure for input/output/cohort slots
    * Randomly choose to iterate over slots in different orders, to avoid hammering the lower slots
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
