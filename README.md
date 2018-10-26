# ewcc
Making a compiler for learning purposes focusing on simplicity and correctness.

## Current Progress:
The compiler can currently take a program with a small subset of C, like:

```C
int main(int argc, char **argv)
{
	for (int i = 0; i < argc; i++) {
		argv[i][0] = i;
	}
}
```
And parses it to generate an AST, like:

```
AST Tree View
------------------------------------
func main: fcn( argc: int, argv: ptr(ptr(char))) -> int
 `--block:
   `--loop:
     `--decl:int i
     | `--cnum: 0
     `--binop '<':
     | `--ident: i
     | `--ident: argc
     `--unop '++'
     | `--ident: i
     `--block:
       `--binop '=':
         `--unop '*'
         | `--binop '+':
         |   `--unop '*'
         |   | `--binop '+':
         |   |   `--ident: argv
         |   |   `--ident: i
         |   `--cnum: 0
         `--ident: i

------------------------------------
```
Lastly, it generates Three-Address-Code as an IR representation, and is currently very unoptimized:

```
TAC Intermediate Representation
------------------------------------
	alloc _i
	ST [_i], #0
L0
	LD R0, [_i]
	LD R1, [_argc]
	R0 < R1
	ifneq L2
	goto L1
L1
	LD R2, [_i]
	LD R3, [_argv]
	LD R4, [_i]
	R5 = R3 + R4
	LD R6, [R5]
	R7 = R6 + #0
	LD R8, [R7]
	ST [R8], R2
	LD R9, [_i]
	R10 = R9 + #1
	ST [_i], R10
	goto L0
L2
------------------------------------

```

Now finishing up the parser and IR representation, then will start the code generation.
Missing random parts that I am filling in:
- string & character literals
- do/while and switch
- random operators
