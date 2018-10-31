#ifndef X86_GEN_H
#define X86_GEN_H

#include "match.h"
#include "debug.h"
#include "frontend/symbols.h"

/*Code generation for X86*/
/*Target architectures must implement:
 *
 * Local variable allocator:
 * Instructions that implement all statements with possible operand pairs
 *
 * The variable allocator is a callback
 *
 * Needed Data:
 * Register list
 * ABI: Function parameter passing, function register clobbering/saving
 *
 * Instruction list
 * */

//mov [stmt_store]
#define PTR_SIZE 4
#define NUM_GREGS 6
#define NUM_REGS 32
#ifndef STMT_PAT
#define STMT_PAT(flag, type, r, a1, a2) {flag, type, r, a1, a2}
#endif

extern const char *general_registers[NUM_REGS];
extern const char *usable_regs[NUM_GREGS];
/*X86 Instructions Must mark clobbered registers*/
/* EAX | ECX | EDX | EBX | ESP | EBP | ESI | EDI*/
/*  1  	  2     4     8     16   32     64   128*/
#define EAX 0x1
#define ECX 0x2
#define EDX 0x4
#define EBX 0x8
#define ESP 0x10
#define EBP 0x20
#define ESI 0x40
#define EDI 0x80

struct x86ins {
	/*Instruction format string*/
	char *fmt;
	/*Registers that are not explicitly used, but their value is changed by the instr*/
	unsigned long clobber;
};

#define INS(fmt, used) {fmt, used}
/*Instruction formatting:
 * $operand_number or if invalid number, label is used
 * | another instruction
 * > go to next stmt
 * < go to prev stmt
 * */

#define INS_INSTR\
	/*Allocate $1 bytes on the stack*/\
	INS("sub esp, $1", 		0),\
	/*Move*/\
	INS("mov $0, $1", 		0),\
	/*Addition: $0 = $0 + $2*/\
	INS("add $0, $2", 		0),\
	/*Subtraction: $0 = $0 + $2*/\
	INS("sub $0, $2", 		0)\
	/*Multiply: $0 = $1 + $2 (assert $0 and $1 == eax, edx clobbered)*/\
	INS("imul $1, $2", 		EDX),\
	/*Divide: $0 = $0 / $2 (assert $0 == eax, clobbered edx)*/\
	INS("idiv $1", 			EDX),\
	/*dw "Store" instructions*/\
	INS("mov dword [$0], $1", 	0),\
	/*Load instructions*/\
	INS("mov $0, dword [$0]", 	0),\
	/*Index load: R* = R* + idx -> LD any, [R0] */\
	INS(">mov $0, [$1+<$2]",  	0),\
	/*Comparison and jumps*/\
	INS("cmp $1, $2|>jl L$l", 	0),\
	INS("cmp $1, $2|>jle L$l", 	0),\
	INS("cmp $1, $2|>jg L$l", 	0),\
	INS("cmp $1, $2|>jge L$l", 	0),\
	INS("cmp $1, $2|>je L$l", 	0),\
	INS("cmp $1, $2|>jne L$l", 	0),\
	INS("jmp $l", 			0),\
	INS("ret", 		  	0),\


#define INS_TABLE 	\
	PATT(1, 0, INS("sub esp, $1", 	0),\
		STMT_PAT(0, stmt_alloc, IGN, IGN, IGN))\
	PATT(1, 1, INS("add $0, $2", 0),\
		STMT_PAT(0, stmt_add, REG, RES, IMM))\
	PATT(1, 2, INS("imul $1, $2", 	EDX),\
		STMT_PAT(0, stmt_mul, REG, REG, IMM))\
/*
	PATT(1, "idiv ebx",\
		STMT_PAT(0, stmt_div, REG, REG, IMM))\
	PATT(1, "mov dword [$0], $1",\
		STMT_PAT(0, stmt_store, MEM, REG, IGN))\
	PATT(1, "mov dword [$0], $1",\
		STMT_PAT(0, stmt_store, MEM, IMM, IGN))\
	PATT(1, "mov $0, $1",\
		STMT_PAT(0, stmt_move, REG, REG, IGN))\
	PATT(1, "ret",\
		STMT_PAT(0, stmt_ret, IGN, IGN, IGN))\
	PATT(1, "mov $0, dword [$1]",\
		STMT_PAT(0, stmt_load, REG, MEM, IGN))\
	PATT(1, "jmp L$l",\
		STMT_PAT(0, stmt_ugoto, IGN, IGN, IGN))\
	PATT(1, "L$l:",\
		STMT_PAT(0, stmt_label, IGN, IGN, IGN))\
	PATT(2, "cmp $1, $2|^jl L$l",\
		STMT_PAT(0, stmt_lt, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(2, "cmp $1, $2|^jle L$l",\
		STMT_PAT(0, stmt_lte, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
*/

//#define NUM_IPSTMT 15
//#define NUM_IPATTERNS 13
#define NUM_IPSTMT 3
#define NUM_IPATTERNS 3

extern struct x86ins instructions[NUM_IPATTERNS];
extern struct stmt_pattern ins_pattern_stmts[NUM_IPSTMT];
extern struct pattern ins_patterns[NUM_IPATTERNS];

void instr(struct ir_stmt *start);
struct ir_stmt *find_instr(struct ir_stmt *cur, void *val);
void fmt_ins(const char *str, struct ir_stmt *stmt);
void fmt_opr(struct ir_operand *oper);

/*
#define INSTR_TAB 		\
	MATCH(1,"add $0, $1", 	\
		STMT_MATCH(stmt_add, reg, result, imm))\

{"sub esp, $0", 	stmt_alloc, 	{SIZEOF()}}
{"add $0, $1", 		stmt_add, 	{REG(), OPR(0), REG()}}
{"mov [$0], $1", 	stmt_store, 	{ADDR(), VAL()}},
{"mov [$0], $1", 	stmt_store, 	{ADDR(), VAL()}},
{"mov eax, %0", 	stmt_retval, 	{VAL()}},
{"ret", 		stmt_ret, 	{}},
*/

int 		x86_sizeof(struct type *type);
unsigned long 	x86_local(struct symbol *s);

#endif
