# ewcc
Making a compiler for learning purposes focusing on simplicity and correctness.

## Current Progress:
The compiler can currently take a program with a small subset of C, like (:

```C
int fib(int n) {
	if (n <= 1) return n;
	else {
		return fib(n-1) + fib(n-2);
	}
}
int main() {
	for (int i = 0; i < 10; i++) {
		int v = fib(i);
	}
	return 0;
}
```

Then it will parse it to an AST, and do some lazy type and semantic checking (for now), then after inserting
any needed implicit casts, it emits:

```
AST Tree View
------------------------------------
func fib: fcn( n: int) -> int
 `--block:
   `--conditional:
    |`--binop '<=':
    |  `--ident: n
    |  `--cnum: 1
    |`--return
    |  `--ident: n
     `--block:
       `--return
         `--binop '+':
           `--call
           | `--ident: fib
           | `--binop '-':
           |   `--ident: n
           |   `--cnum: 1
           `--call
             `--ident: fib
             `--binop '-':
               `--ident: n
               `--cnum: 2
func main: fcn( ) -> int
 `--block:
   `--loop:
   | `--decl:int i
   | | `--cnum: 0
   | `--binop '<':
   | | `--ident: i
   | | `--cnum: 10
   | `--unop '++'
   | | `--ident: i
   | `--block:
   |   `--decl:int v
   |     `--call
   |       `--ident: fib
   |       `--ident: i
   `--return
     `--cnum: 0


------------------------------------
```

Then it generates Three Address Code, which is mostly machine independent, except using the type sizes given to it
in a machine definition file. It currently makes no assumptions about ABI's, and doesn't resolve global or function arguments.
Local variables are stored on the stack as an offset from a frame pointer. The first pass uses infinite temporary registers,
and creates very unoptimized code.

```
TAC Intermediate Representation
------------------------------------
 1: 	LD R0, dw[_n]
 2: 	R0 > #1
 3: 	if true L1
 4: L0
 5: 	LD R1, dw[_n]
 6: 	retval R1
 7: 	goto L2
 8: 	goto L2
 9: L1
10: 	LD R3, dw[_n]
11: 	R4 = R3 - #1
12: 	param_#0 R4
13: 	R2 = _fib()
14: 	LD R6, dw[_n]
15: 	R7 = R6 - #2
16: 	param_#0 R7
17: 	R5 = _fib()
18: 	R8 = R2 + R5
19: 	retval R8
20: 	goto L2
21: L2
22: 	ret
# This is the start of the main function, currently it doesn't emit function headers
23: 	alloc fp-4
24: 	ST dw[fp-4], #0
25: 	goto L4
26: L5
27: 	alloc fp-8
28: 	LD R10, dw[fp-4]
29: 	param_#0 R10
30: 	R9 = _fib()
31: 	ST dw[fp-8], R9
32: 	LD R11, dw[fp-4]
33: 	R12 = R11 + #1
34: 	ST dw[fp-4], R12
35: L4
36: 	LD R13, dw[fp-4]
37: 	R13 < #10
38: 	if true L5
39: L6
40: 	retval #0
41: L7
42: 	ret
------------------------------------

```

Then it will construct a control flow graph, and uses a local register allocation algorithm to rename registers (for now it doesn't touch variables, so it is not really register allocation). Then it dumps the interference graph, along with the final colored registers.

```
------------------------------------
Interference Graph
------------------------------------
R0 is defined at 1 and killed at 2
R1 is defined at 5 and killed at 6
R3 is defined at 10 and killed at 11
R4 is defined at 11 and killed at 12
R2 is defined at 13 and killed at 18
R6 is defined at 14 and killed at 15
R7 is defined at 15 and killed at 16
R5 is defined at 17 and killed at 18
R8 is defined at 18 and killed at 19
R2 interferes with R6
R2 interferes with R7
R2 interferes with R5
R10 is defined at 28 and killed at 29
R9 is defined at 30 and killed at 31
R11 is defined at 32 and killed at 33
R12 is defined at 33 and killed at 34
R13 is defined at 36 and killed at 37
Assigned R0 -> 0
Assigned R1 -> 0
Assigned R13 -> 0
Assigned R12 -> 0
Assigned R11 -> 0
Assigned R9 -> 0
Assigned R10 -> 0
Assigned R8 -> 0
Assigned R5 -> 0
Assigned R7 -> 0
Assigned R6 -> 0
Assigned R2 -> 1
Assigned R4 -> 0
Assigned R3 -> 0
------------------------------------
```

Then it renames the registers, and does some basic peephole optimizations on the code, then emits the Three Address Code again.

```
------------------------------------
After Register Rewriting:
 1: 	LD R0, dw[_n]
 2: 	R0 > #1
 3: 	if true L1
 4: L0
 5: 	LD R0, dw[_n]
 6: 	retval R0
 7: 	goto L2
 8: L1
 9: 	LD R0, dw[_n]
10: 	R0 = R0 - #1
11: 	param_#0 R0
12: 	R1 = _fib()
13: 	LD R0, dw[_n]
14: 	R0 = R0 - #2
15: 	param_#0 R0
16: 	R0 = _fib()
17: 	R0 = R1 + R0
18: 	retval R0
19: L2
20: 	ret
# main() starts here
21: 	alloc fp-4
22: 	ST dw[fp-4], #0
23: 	goto L4
24: L5
25: 	alloc fp-8
26: 	LD R0, dw[fp-4]
27: 	param_#0 R0
28: 	R0 = _fib()
29: 	ST dw[fp-8], R0
30: 	LD R0, dw[fp-4]
31: 	R0 = R0 + #1
32: 	ST dw[fp-4], R0
33: L4
34: 	LD R0, dw[fp-4]
35: 	R0 < #10
36: 	if true L5
37: L6
38: 	retval #0
39: L7
40: 	ret
------------------------------------
```

Missing random features, and the codebase is too unstable to document them all. 
Will be adding a basic X86 Instruction Selection module soon.
