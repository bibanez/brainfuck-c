all: brainfuck compiler

brainfuck: brainfuck.c
	gcc -O3 -o brainfuck brainfuck.c

compiler: compiler.c
	gcc -O3 -o compiler compiler.c

clear:
	rm brainfuck compiler