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
/*		STMT_PAT(DST, stmt_load, REG, MEM, IGN),\
		STMT_PAT(SRC, stmt_add, RES, RES, IGN),\
		STMT_PAT(SRC, stmt_store, IGN, AG1, RES))\
*/

#define INS_TABLE 	\
	PATT(1, 0, INS("sub esp, $1", 	0),\
		STMT_PAT(0, stmt_alloc, IGN, IGN, IGN))\
	PATT(3, 1, INS("add @1 [$1], >$2", 0),\
		STMT_PAT(SRC, stmt_load, AG2, AG1, IGN),\
		STMT_PAT(SRC, stmt_add, AG2, AG2, IGN),\
		STMT_PAT(DST, stmt_store, IGN, MEM, REG))\
	PATT(1, 2, INS("imul $1, $2", 	EDX),\
		STMT_PAT(0, stmt_mul, REG, REG, IMM))\
	PATT(1, 3, INS("mov @1[$1],  $2", 0),\
		STMT_PAT(0, stmt_store, IGN, IGN, IGN))\
	PATT(1, 4, INS("mov $0, @1[$1]",  0),\
		STMT_PAT(0, stmt_load, REG, IGN, IGN))\
	PATT(1, 5, INS("L$l:", 0),\
		STMT_PAT(0, stmt_label, IGN, IGN, IGN))\
	PATT(1, 6, INS("jmp L$l",  0),\
		STMT_PAT(0, stmt_ugoto, IGN, IGN, IGN))\
	PATT(2, 7, INS("cmp $1, $2|>jl L$l", 0),\
		STMT_PAT(0, stmt_lt, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(2, 8, INS("cmp $1, $2|>jle L$l", 0),\
		STMT_PAT(0, stmt_lte, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(1, 9, INS("mov eax, $1", 0),\
		STMT_PAT(0, stmt_retval, IGN, IGN, IGN))\
	PATT(1, 10, INS("push $2", 0),\
		STMT_PAT(0, stmt_param, IGN, IGN, IGN))\
	PATT(1, 11, INS("ret", 0),\
		STMT_PAT(0, stmt_ret, IGN, IGN, IGN))\
	PATT(2, 12, INS("cmp $1, $2|>jg L$l", 0),\
		STMT_PAT(0, stmt_gt, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(2, 13, INS("cmp $1, $2|>jge L$l", 0),\
		STMT_PAT(0, stmt_gte, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(2, 14, INS("cmp $1, $2|>jne L$l", 0),\
		STMT_PAT(0, stmt_neq, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(2, 15, INS("cmp $1, $2|>je L$l", 0),\
		STMT_PAT(0, stmt_eq, IGN, REG, IGN),\
		STMT_PAT(0, stmt_cgoto, IGN, IGN, IGN))\
	PATT(1, 16, INS("lea $0, [$1]", 0),\
		STMT_PAT(0, stmt_addr, REG, IGN, IGN))\
	PATT(1, 17, INS("add $0, $1", 0),\
		STMT_PAT(0, stmt_add, REG, IGN, RES))\
	PATT(1, 18, INS("add $0, $2", 0),\
		STMT_PAT(0, stmt_add, REG, RES, IGN))\
	PATT(1, 19, INS("sub $0, $2", 0),\
		STMT_PAT(0, stmt_sub, REG, RES, IGN))\
	PATT(1, 20, INS("mov $0, $1|add $0, $2", 0),\
		STMT_PAT(0, stmt_add, REG, REG, IGN))\
	PATT(1, 21, INS("$1:", 0),\
		STMT_PAT(0, stmt_func, IGN, IGN, IGN))\
	PATT(1, 22, INS("call $1|mov $0, eax", 0),\
		STMT_PAT(0, stmt_call, IGN, IGN, IGN))\
	PATT(1, 23, INS("mov $0, $1", 		0),\
		STMT_PAT(0, stmt_move, IGN, IGN, IGN))

#define NUM_IPSTMT 32
#define NUM_IPATTERNS 24

extern struct x86ins instructions[NUM_IPATTERNS];
extern struct stmt_pattern ins_pattern_stmts[NUM_IPSTMT];
extern struct pattern ins_patterns[NUM_IPATTERNS];

void instr(struct ir_stmt *start);
struct ir_stmt *find_instr(struct ir_stmt *cur, int ns, int idx);
void fmt_ins(const char *str, struct ir_stmt *stmt);
void fmt_opr(struct ir_operand *oper);

#endif
