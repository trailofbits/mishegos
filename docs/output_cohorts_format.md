Cohort Output Format
=====================

This file briefly describes the format of `mishegos`'s binary output.

The details of the binary format are an implementation detail and should only be of interest
if working on mishegos itself; users looking to analyze mishegos's results should run
`mish2jsonl` and operate on the JSONL-formatted results.

## Motivation

Earlier versions of mishegos dumped their results directly to JSONL. This required
us to do JSON serialization and internal allocations in the fuzzing lifecycle, incurring
a performance hit.

## Format

Mishegos's binary output is a sequence of "cohorts", each of which contains `N` outputs
where `N` is the number of workers.

Each cohort begins with a header:

* `nworkers` (`u32`): The number of workers present in this output cohort
* `input` (`u64` + `str`): A length-prefixed, pretty-printed hex string of the input handled by
this cohort

After the header, each cohort contains `nworkers` output records. Each output contains:

* `status` (`u32`): A status code corresponding to the `decode_status` enum
* `ndecoded` (`u16`): The number of bytes of `input` decoded
* `workerno` (`u32`): The worker's identifying index
* `worker_so` (`u64` + `str`): A length-prefixed string containg the path to the worker's dynamic
shared object
* `len` (`u16`): The string length of the decoded instruction, or `0` if none is present
* `result` (`str`): A string of `len` bytes containing the decoded instruction

Visualized:

```
|-------------------------|
|  cohort 1: nworkers: 3  |
|    output 1             |
|    output 2             |
|    output 3             |
|-------------------------|
|  cohort 2: nworkers: 3  |
|    output 1             |
|    output 2             |
|    output 3             |
|-------------------------|
|  cohort ...             |
|    ....                 |
|_________________________|
```

## Implementation

Mishego's binary output is transformed into JSONL via a parser specified in
[Kaitai Struct](https://kaitai.io/)'s DSL.
