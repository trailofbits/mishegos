Dump Cohort Output Format
========================

This page explains the format of the file outputed when running mishegos.c. In order to improve the performance of mishegos the format of the cohort output changed the output format from JSON to dumpig the cohort structs directly.

Dumping the cohorts struct directly rewuires less serialization work and fewre allocations. In a seprate program called ___  can convert back to the JSON format. Inorder to be able to convert the file back into a JSON must know the format with the length and version prefix the structs.

##Format of the File

The file is organized in the following way for each cohort:

* `nworkers` numbers of workers that will iterate through in that cohort
* `input`
* `output` the output for each worker
*  Each worker outputs:
	* `status` tells the decode status
		* `value` 0-7 represting the name
		* `name` tells if status is none, success, failure, crash, partial, wouldblock, unknown
	* `ndecoded` number of bytes decoded for the instruction
	* `workerno` which number of worker on
	* `worker_so` the path to the worker
	* `len` the string length of the decoded instruction
	* `result` the output


## Example of Output File - Usage

### Building

As discussed in the mishegos.md file. `mishegos` is most easily build within Docker:
that
```bash
docker build -t mishegos .
```
Alternatively, you can try building directly. See the README.md,

###Running

Run the fuzzer for a bit by:

```bash
./src/mishegos/mishegos ./workers.spec
```

`mishegos` checks for three environment variables:

* `V=1` enables verbose output on `stderr`
* `D=1` enables the "dummy" mutation mode for debugging purposes
* `M=1` enables the "manual" mutation mode (i.e., read from `stdin`)


Run the example given below

```bash
 M=1 ./src/mishegos/mishegos ./workers.spec <<< "36f03e4efd" > /tmp/mishego
```

### Example Output
When opening up the `./tmp/mishegos` file as a hexdump (which can be done
through `xxd mishegos`).  The file should show this output:

```
00000000: 0a00 0000 0000 0000 3336 6630 3365 3465  ........36f03e4e
00000010: 6664 0100 0000 0500 0000 0000 1700 0000  fd..............
00000020: 0000 0000 2e2f 7372 632f 776f 726b 6572  ...../src/worker
00000030: 2f62 6664 2f62 6664 2e73 6f18 0073 7320  /bfd/bfd.so..ss
00000040: 6c6f 636b 2064 7320 7265 782e 5752 5820  lock ds rex.WRX
00000050: 7374 6420 0a02 0000 0000 0001 0000 0021  std ...........!
00000060: 0000 0000 0000 002e 2f73 7263 2f77 6f72  ......../src/wor
00000070: 6b65 722f 6361 7073 746f 6e65 2f63 6170  ker/capstone/cap
00000080: 7374 6f6e 652e 736f 0000 0200 0000 0000  stone.so........
00000090: 0200 0000 2300 0000 0000 0000 2e2f 7372  ....#......../sr
000000a0: 632f 776f 726b 6572 2f64 796e 616d 6f72  c/worker/dynamor
000000b0: 696f 2f64 796e 616d 6f72 696f 2e73 6f00  io/dynamorio.so.
000000c0: 0002 0000 0000 0003 0000 001b 0000 0000  ................
000000d0: 0000 002e 2f73 7263 2f77 6f72 6b65 722f  ..../src/worker/
000000e0: 6661 6465 632f 6661 6465 632e 736f 0000  fadec/fadec.so..
000000f0: 0100 0000 0500 0400 0000 1d00 0000 0000  ................
00000100: 0000 2e2f 7372 632f 776f 726b 6572 2f75  .../src/worker/u
00000110: 6469 7338 362f 7564 6973 3836 2e73 6f08  dis86/udis86.so.
00000120: 006c 6f63 6b20 7374 6402 0000 0000 0005  .lock std.......
00000130: 0000 0017 0000 0000 0000 002e 2f73 7263  ............/src
00000140: 2f77 6f72 6b65 722f 7865 642f 7865 642e  /worker/xed/xed.
00000150: 736f 0000 0200 0000 0000 0600 0000 1b00  so..............
00000160: 0000 0000 0000 2e2f 7372 632f 776f 726b  ......./src/work
00000170: 6572 2f7a 7964 6973 2f7a 7964 6973 2e73  er/zydis/zydis.s
00000180: 6f00 00                                  o..
```

### Converting

Once doubled chekced the file is in the correct format. Can start converting the dumped cohorts to JSON format.

Building mishegos generated the cohort file from the kaitai____source____code. The cohort file created can run the `setup` file to convert a given cohort dump file to a json file, which can be run with the above example in the container.

```bash
python3 src/mish2json/setup.py </tmp/mishegos >/tmp/mishegos_json
```

The new file should look as follows:

```
{"nworkers": 7, "inputs": "36f03e4efd", "outputs": [{"status": {"value": 1, "name": "S_SUCCESS"}, "ndecoded": 5, "workerno": 0, "worker_so": "./src/worker/bfd/bfd.so", "len": 24, "result": "ss lock ds rex.WRX std \n"}, {"status": {"value": 2, "name": "S_FAILURE"}, "ndecoded": 0, "workerno": 1, "worker_so": "./src/worker/capstone/capstone.so", "len": 0, "result": ""}, {"status": {"value": 2, "name": "S_FAILURE"}, "ndecoded": 0, "workerno": 2, "worker_so": "./src/worker/dynamorio/dynamorio.so", "len": 0, "result": ""}, {"status": {"value": 2, "name": "S_FAILURE"}, "ndecoded": 0, "workerno": 3, "worker_so": "./src/worker/fadec/fadec.so", "len": 0, "result": ""}, {"status": {"value": 1, "name": "S_SUCCESS"}, "ndecoded": 5, "workerno": 4, "worker_so": "./src/worker/udis86/udis86.so", "len": 8, "result": "lock std"}, {"status": {"value": 2, "name": "S_FAILURE"}, "ndecoded": 0, "workerno": 5, "worker_so": "./src/worker/xed/xed.so", "len": 0, "result": ""}, {"status": {"value": 2, "name": "S_FAILURE"}, "ndecoded": 0, "workerno": 6, "worker_so": "./src/worker/zydis/zydis.so", "len": 0, "result": ""}]}

```
