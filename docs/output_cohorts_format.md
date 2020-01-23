Dump Cohort Output Format 
========================

This page explains the format of the file outputed when running mishegos.c. In order to improve the performance of mishegos the format of the cohort output changed the output format from JSON to dumpig the cohort structs directly. 

Dumping the cohorts struct directly rewuires less serialization work and fewre allocations. In a seprate program called ___  can convert back to the JSON format. Inorder to be able to convert the file back into a JSON must know the format with the length and version prefix the structs. 

##Format of the File 

The file is organized in the following way: 

* `nworkers` numbers of workers that will iterate through
* `input` ? what does this do 
* `input` other stuff in the input struct
* For each worker there is: 
	* `status` 0 or 1 
	* `status_name` if status is 0 than unsuccessful and if status is 1 than successful
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

###Example Output
When opening up the `./tmp/mishegos` file in a text editor should see the following output: 

```
36f03e4efd./src/worker/bfd/bfd.soss lock ds rex.WRX std 
!./src/worker/capstone/capstone.so#.src/worker/dynamorio/dynamorio.so./src/worker/fadec.so./src/worker/udis86/udis86.solock std./src/worker/xed/xed.so./src/worker/zydis.so

```

Then open up the file in hexdump for the file to make sure that very byte is correct. This can be done by obeing up the `./tmp/mishegos` through `xxd mishegos`.  Terminal should show this output: 


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
Run the _ progrma to get the file back into the JSON format which output looks like this: 

```
{"input":"36f03e4efd","outputs":[{"ndecoded":5,"len":24,"result":"ss lock ds rex.WRX std \n","workerno":0,"status":1,"status_name":"success","worker_so":"./src/worker/bfd/bfd.so"},{"ndecoded":0,"len":0,"result":null,"workerno":1,"status":2,"status_name":"failure","worker_so":"./src/worker/capstone/capstone.so"},{"ndecoded":0,"len":0,"result":null,"workerno":2,"status":2,"status_name":"failure","worker_so":"./src/worker/dynamorio/dynamorio.so"},{"ndecoded":0,"len":0,"result":null,"workerno":3,"status":2,"status_name":"failure","worker_so":"./src/worker/fadec/fadec.so"},{"ndecoded":5,"len":8,"result":"lock std","workerno":4,"status":1,"status_name":"success","worker_so":"./src/worker/udis86/udis86.so"},{"ndecoded":0,"len":0,"result":null,"workerno":5,"status":2,"status_name":"failure","worker_so":"./src/worker/xed/xed.so"},{"ndecoded":0,"len":0,"result":null,"workerno":6,"status":2,"status_name":"failure","worker_so":"./src/worker/zydis/zydis.so"}]}
```

In hex dump form the file looks like what follows: 
	
```
00000000: 7b22 696e 7075 7422 3a22 3336 6630 3365  {"input":"36f03e
00000010: 3465 6664 222c 226f 7574 7075 7473 223a  4efd","outputs":
00000020: 5b7b 226e 6465 636f 6465 6422 3a35 2c22  [{"ndecoded":5,"
00000030: 6c65 6e22 3a32 342c 2272 6573 756c 7422  len":24,"result"
00000040: 3a22 7373 206c 6f63 6b20 6473 2072 6578  :"ss lock ds rex
00000050: 2e57 5258 2073 7464 205c 6e22 2c22 776f  .WRX std \n","wo
00000060: 726b 6572 6e6f 223a 302c 2273 7461 7475  rkerno":0,"statu
00000070: 7322 3a31 2c22 7374 6174 7573 5f6e 616d  s":1,"status_nam
00000080: 6522 3a22 7375 6363 6573 7322 2c22 776f  e":"success","wo
00000090: 726b 6572 5f73 6f22 3a22 2e2f 7372 632f  rker_so":"./src/
000000a0: 776f 726b 6572 2f62 6664 2f62 6664 2e73  worker/bfd/bfd.s
000000b0: 6f22 7d2c 7b22 6e64 6563 6f64 6564 223a  o"},{"ndecoded":
000000c0: 302c 226c 656e 223a 302c 2272 6573 756c  0,"len":0,"resul
000000d0: 7422 3a6e 756c 6c2c 2277 6f72 6b65 726e  t":null,"workern
000000e0: 6f22 3a31 2c22 7374 6174 7573 223a 322c  o":1,"status":2,
000000f0: 2273 7461 7475 735f 6e61 6d65 223a 2266  "status_name":"f
00000100: 6169 6c75 7265 222c 2277 6f72 6b65 725f  ailure","worker_
00000110: 736f 223a 222e 2f73 7263 2f77 6f72 6b65  so":"./src/worke
00000120: 722f 6361 7073 746f 6e65 2f63 6170 7374  r/capstone/capst
00000130: 6f6e 652e 736f 227d 2c7b 226e 6465 636f  one.so"},{"ndeco
00000140: 6465 6422 3a30 2c22 6c65 6e22 3a30 2c22  ded":0,"len":0,"
00000150: 7265 7375 6c74 223a 6e75 6c6c 2c22 776f  result":null,"wo
00000160: 726b 6572 6e6f 223a 322c 2273 7461 7475  rkerno":2,"statu
00000170: 7322 3a32 2c22 7374 6174 7573 5f6e 616d  s":2,"status_nam
00000180: 6522 3a22 6661 696c 7572 6522 2c22 776f  e":"failure","wo
00000190: 726b 6572 5f73 6f22 3a22 2e2f 7372 632f  rker_so":"./src/
000001a0: 776f 726b 6572 2f64 796e 616d 6f72 696f  worker/dynamorio
000001b0: 2f64 796e 616d 6f72 696f 2e73 6f22 7d2c  /dynamorio.so"},
000001c0: 7b22 6e64 6563 6f64 6564 223a 302c 226c  {"ndecoded":0,"l
000001d0: 656e 223a 302c 2272 6573 756c 7422 3a6e  en":0,"result":n
000001e0: 756c 6c2c 2277 6f72 6b65 726e 6f22 3a33  ull,"workerno":3
000001f0: 2c22 7374 6174 7573 223a 322c 2273 7461  ,"status":2,"sta
00000200: 7475 735f 6e61 6d65 223a 2266 6169 6c75  tus_name":"failu
00000210: 7265 222c 2277 6f72 6b65 725f 736f 223a  re","worker_so":
00000220: 222e 2f73 7263 2f77 6f72 6b65 722f 6661  "./src/worker/fa
00000230: 6465 632f 6661 6465 632e 736f 227d 2c7b  dec/fadec.so"},{
00000240: 226e 6465 636f 6465 6422 3a35 2c22 6c65  "ndecoded":5,"le
00000250: 6e22 3a38 2c22 7265 7375 6c74 223a 226c  n":8,"result":"l
00000260: 6f63 6b20 7374 6422 2c22 776f 726b 6572  ock std","worker
00000270: 6e6f 223a 342c 2273 7461 7475 7322 3a31  no":4,"status":1
00000280: 2c22 7374 6174 7573 5f6e 616d 6522 3a22  ,"status_name":"
00000290: 7375 6363 6573 7322 2c22 776f 726b 6572  success","worker
000002a0: 5f73 6f22 3a22 2e2f 7372 632f 776f 726b  _so":"./src/work
000002b0: 6572 2f75 6469 7338 362f 7564 6973 3836  er/udis86/udis86
000002c0: 2e73 6f22 7d2c 7b22 6e64 6563 6f64 6564  .so"},{"ndecoded
000002d0: 223a 302c 226c 656e 223a 302c 2272 6573  ":0,"len":0,"res
000002e0: 756c 7422 3a6e 756c 6c2c 2277 6f72 6b65  ult":null,"worke
000002f0: 726e 6f22 3a35 2c22 7374 6174 7573 223a  rno":5,"status":
00000300: 322c 2273 7461 7475 735f 6e61 6d65 223a  2,"status_name":
00000310: 2266 6169 6c75 7265 222c 2277 6f72 6b65  "failure","worke
00000320: 725f 736f 223a 222e 2f73 7263 2f77 6f72  r_so":"./src/wor
00000330: 6b65 722f 7865 642f 7865 642e 736f 227d  ker/xed/xed.so"}
00000340: 2c7b 226e 6465 636f 6465 6422 3a30 2c22  ,{"ndecoded":0,"
00000350: 6c65 6e22 3a30 2c22 7265 7375 6c74 223a  len":0,"result":
00000360: 6e75 6c6c 2c22 776f 726b 6572 6e6f 223a  null,"workerno":
00000370: 362c 2273 7461 7475 7322 3a32 2c22 7374  6,"status":2,"st
00000380: 6174 7573 5f6e 616d 6522 3a22 6661 696c  atus_name":"fail
00000390: 7572 6522 2c22 776f 726b 6572 5f73 6f22  ure","worker_so"
000003a0: 3a22 2e2f 7372 632f 776f 726b 6572 2f7a  :"./src/worker/z
000003b0: 7964 6973 2f7a 7964 6973 2e73 6f22 7d5d  ydis/zydis.so"}]
000003c0: 7d0a                                     }.
```

