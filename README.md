# not_python -- an experimental programming language

I wanted to make a compiled programming language with the syntax of python.

This is currently a work in progress, and I am currently focused on other projects.

## Trying the project

If you wish to try the project out, know that it currently: 

- only works on Linux
- is still missing many features (see `test/features` for supported code snippets)

### Building

The project relies on libc and having a C compiler available at `cc` (tested with gcc)

To make the compiler clone the repo and navigate to it's directory then run:

```sh
make debug
```

This will create the `npc` compiler program

### Usage

given a file `main.py`:

```py
# main.py
print("Hello, world")
```

```sh
> ./npc main.py
> ./main
Hello, world

> ./npc main.py -o out.bin
> ./out.bin
Hello, world

./npc main.py --run
Hello, world
# note this still creates the `main` binary file
```

### Testing

You can also run the test suite (relies on python3):
```sh
make test
```
