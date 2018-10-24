# ewcc
Small C compiler in development in C99 with a handwritten Recursive Descent parser
Making a compiler for learning purposes focusing on simplicity and correctness.
## Current Progress:
Parser currently handles a small subset of C and produces type information, a symbol table, and an AST.
Example:
```C
int main(int argc, char **argv)  
{
	for (int i = 0; i < argc; i++) {
		argv[i][0] = i;
	}
}
```
Produces the output:
```

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
```
Now working on semantic error checking, IR, and code-gen.
