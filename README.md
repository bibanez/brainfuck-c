# Brainfuck-C

My simple bf interpreter. Plus transpiler to C.

## Usage

1. `make`

### Interpreter

2. `./brainfuck file_in`

### Compiler

3. `./compiler file_in [-o file_out]`

## Performance

On my machine (i7-10710U CPU). Left interpreted, right compiled:

Program | Time (s)
----|----
mandel | 10.356/1.544
hanoi | 12.649/0.058
bench | 0.803/0.033