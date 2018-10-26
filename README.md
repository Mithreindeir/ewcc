# ewcc
Small C compiler in development in C99 with a handwritten Recursive Descent parser
Making a compiler for learning purposes focusing on simplicity and correctness.
## Current Progress:
Example:
```C
int main(int argc, char **argv)
{
	for (int i = 0; i < argc; i++) {
		argv[i][0] = i;
	}
}
```
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
Now working on semantic error checking, IR, and code-gen.
