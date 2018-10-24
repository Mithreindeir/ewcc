# ewcc
Small C compiler in development in C99 with a handwritten Recursive Descent parser
Making a compiler for learning purposes focusing on simplicity and correctness.
## Current Progress:
Parser currently handles a small subset of C and produces type information, a symbol table, and an AST.
Examples:
```C
int main(int argc, char **argv)  
{
	for (int i = 0; i < argc; i++) {
		argv[i][0] = i;
	}
}
```
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

```C
int foo() {
	int a = 2;
	int b = 8;
	int c = 4;
	int d;
	for (int j = 0; j <= 10; j++) {
		a = a * (j * (b / c));
		d = a * (j * (b / c));
	}
	if (a > b) {
		b = a - b;
	} else if (a < d) {
		c = d - a + b / 2;
	}
}

```
```
func foo: fcn( ) -> int
 `--block:
   `--decl:int a
   | `--cnum: 2
   `--decl:int b
   | `--cnum: 8
   `--decl:int c
   | `--cnum: 4
   `--decl:int d
   `--loop:
   | `--decl:int j
   | | `--cnum: 0
   | `--binop '<=':
   | | `--ident: j
   | | `--cnum: 10
   | `--unop '++'
   | | `--ident: j
   | `--block:
   |   `--binop '=':
   |   | `--ident: a
   |   | `--binop '*':
   |   |   `--ident: a
   |   |   `--binop '*':
   |   |     `--ident: j
   |   |     `--binop '/':
   |   |       `--ident: b
   |   |       `--ident: c
   |   `--binop '=':
   |     `--ident: d
   |     `--binop '*':
   |       `--ident: a
   |       `--binop '*':
   |         `--ident: j
   |         `--binop '/':
   |           `--ident: b
   |           `--ident: c
   `--conditional:
    |`--binop '>':
    |  `--ident: a
    |  `--ident: b
    |`--block:
    |  `--binop '=':
    |    `--ident: b
    |    `--binop '-':
    |      `--ident: a
    |      `--ident: b
     `--conditional:
      |`--binop '<':
      |  `--ident: a
      |  `--ident: d
       `--block:
         `--binop '=':
           `--ident: c
           `--binop '+':
             `--binop '-':
             | `--ident: d
             | `--ident: a
             `--binop '/':
               `--ident: b
               `--cnum: 2
```
Now working on semantic error checking, IR, and code-gen.
