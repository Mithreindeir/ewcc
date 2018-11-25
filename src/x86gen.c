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
#define PATT(a, idx, ins, ...) {a, idx},
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
	printf(";assembly\n");
	printf("global main\n");
	printf("section .text\n");

	while (start) {
		state.entry = start;
		if (!(start = find_stmt_match(&state))) {
			printf("No match: ");
			ir_stmt_debug(state.entry);
			printf("\n");
			start = state.entry;
		}
		/*prologue*/
		if (start && start->type == stmt_func) {
			printf("\tpush ebp\n");
			printf("\tmov ebp, esp\n");
		}
		start = start->next;
		/*Epilogue*/
		if (start && start->type == stmt_ret) {
			printf("\tleave\n");
		}
	}
}

struct ir_stmt *find_instr(struct ir_stmt *cur, int ns, int idx)
{
	fmt_ins(instructions[idx].fmt, cur);
	while (--ns && cur) cur = cur->next;
	return cur;
}


void fmt_ins(const char *fmt, struct ir_stmt *stmt)
{
	if (!fmt) return;
	int len = strlen(fmt);
	if (stmt->type != stmt_label && stmt->type != stmt_func) printf("\t");
	for (int i = 0; i < len; i++) {
		struct ir_operand *cop[3] = { stmt->result, stmt->arg1, stmt->arg2 };
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
		} else if (fmt[i] == '@' && (i + 1) < len) {
			char c = fmt[i + 1];
			struct ir_operand *o = cop[c-'0'];
			if (o) {
				if (o->size == 1) printf("byte ");
				if (o->size == 2) printf("word ");
				if (o->size == 4) printf("dword ");
			}
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
			signed off = oper->val.sym->offset;
			if (off < 0)
				printf("ebp+%ld", -off);
			else if (off > 0)
				printf("ebp-%ld", off);
			else printf("%s", oper->val.sym->identifier);
			break;
		case oper_cnum: printf("%ld", oper->val.constant); 	break;
		default: break;
	}
}
