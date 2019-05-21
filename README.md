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
