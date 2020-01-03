Dump Cohort Output Format 
========================

This page explains the format of the file outputed when running mishegos.c. In order to improve the performance of mishegos the format of the cohort output changed the output format from JSON to dumpig the cohort structs directly. 

Dumping the cohorts struct directly rewuires less serialization work and fewre allocations. In a seprate program called ___  can covert back to the JSON format. Inorder to be able to convert teh file back into a JSOn must know the format with the length and version premix. 

##Format of the File 

The file is organized in the following way: 

* `nworkers` numbers of workers that will iterate through
* `input` ? what does this do 
* `input` other stuff in the input struct
* For each worker: 
	* `


## Example of Output File - Usage 

### Building

As discussed in the mishegos.md file. `mishegos` is most easily build within Docker: 
that 
```bash
docker build -t mishegos .
```
Alternatively, you can try rbuilding directly. See the README.md,

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
When opening up the `./tmp/mishegos` file in a text editor of the 