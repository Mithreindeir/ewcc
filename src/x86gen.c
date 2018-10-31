#include "x86gen.h"

const char *usable_regs[NUM_GREGS] = {
	"eax", "ecx", "edx", "ebx", "esi", "edi"
};

const char *general_registers[NUM_REGS] = {
	"al", "ax", "eax",
	"cl", "cx", "ecx",
	"dl", "dx", "edx",
	"bl", "bx", "ebx",
	"ah", "sp", "esp",
	"ch", "bp", "ebp",
	"dh", "si", "esi",
	"bh", "di", "edi"
};

struct x86ins instructions[NUM_IPATTERNS] = {
#define PATT(a, idx, ins, ...) [idx]=ins,
	INS_TABLE
#undef PATT
};

struct stmt_pattern ins_pattern_stmts[NUM_IPSTMT] = {
#define PATT(a, idx, ins, ...) __VA_ARGS__,
	INS_TABLE
#undef PATT
};

struct pattern ins_patterns[NUM_IPATTERNS] = {
#define PATT(a, idx, ins, ...) {a, (void*)idx},
	INS_TABLE
#undef PATT
};

void instr(struct ir_stmt *start)
{
	if (!start) return;
	struct pattern_state state;
	state.callback = find_instr;
	state.splist = ins_pattern_stmts;
	state.plist = ins_patterns;
	state.num_spatts = NUM_IPSTMT;
	state.num_patts = NUM_IPATTERNS;
	/*Prologue*/
	printf("; this is just for testing right now\n");
	printf("global main\n");
	printf("section .text\n");
	printf("main:\n");
	printf("\tpush ebp\n");
	printf("\tmov ebp, esp\n");

	while (start) {
		state.entry = start;
		start = find_stmt_match(&state);
		start = start->next;
		/*Epilogue*/
		if (start && start->type == stmt_ret) {
			printf("\tleave\n");
		}
	}
}

struct ir_stmt *find_instr(struct ir_stmt *cur, void *val)
{
	(void)val;
	//fmt_ins(instructions[(int)val].fmt, cur);
	return cur;
}


void fmt_ins(const char *fmt, struct ir_stmt *stmt)
{
	if (!fmt) return;
	struct ir_operand *cop[3] =
	    { stmt->result, stmt->arg1, stmt->arg2 };
	int len = strlen(fmt);
	if (stmt->type != stmt_label) printf("\t");
	for (int i = 0; i < len; i++) {
		if (fmt[i]=='>') {
			stmt = stmt->next;
			if (!stmt) exit(-1);
			continue;
		}
		if (fmt[i]=='<') {
			stmt = stmt->prev;
			if (!stmt) exit(-1);
			continue;
		}
		if (fmt[i]=='|') {
			printf("\n");
			if (stmt->type != stmt_label) printf("\t");
			continue;
		}
		if (fmt[i] == '$' && (i + 1) < len) {
			char c = fmt[i + 1];
			if (c >= '0' && c < '3')
				fmt_opr(cop[fmt[i + 1] - '0']);
			else printf("%d", stmt->label);
			i++;
		} else {
			printf("%c", fmt[i]);
		}
	}
	printf("\n");
}

void fmt_opr(struct ir_operand *oper)
{
	if (!oper) return;
	switch (oper->type) {
		case oper_reg: printf("%s", usable_regs[oper->val.virt_reg]); 	break;
		case oper_sym:
			if (!oper->val.sym) break;
			if (oper->val.sym->offset)
				printf("ebp-%ld", oper->val.sym->offset);
			else printf("%s", oper->val.sym->identifier);
			break;
		case oper_cnum: printf("%ld", oper->val.constant); 	break;
		default: break;
	}
}
