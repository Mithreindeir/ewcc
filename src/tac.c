#include "tac.h"

#define STMT(a, b) b
const char *stmt_str[NUM_STMT] = {
	STMT_TABLE
};
#undef STMT

void debug_fmt(const char *fmt, struct ir_stmt *stmt)
{
	struct ir_operand *cop[3] = {stmt->result, stmt->arg1, stmt->arg2};
	int len = strlen(fmt);
	for (int i = 0; i < len; i++) {
		if (fmt[i]=='$' && (i+1) < len) {
			char c = fmt[i+1];
			if (c>='0'&&c<'3')
				debug_ir_operand(cop[fmt[i+1]-'0']);
			i++;
		} else {
			printf("%c", fmt[i]);
		}
	}
}

void debug_ir_operand(struct ir_operand *oper)
{
	switch (oper->type) {
#define OPER(enumer, fmt, val) case OPER_STR(enumer): printf(fmt, val(oper)); break;
		OPER_TABLE
#undef OPER
	}
}

void debug_ir_stmt(struct ir_stmt *stmt)
{
	const char *fmt = stmt_str[stmt->type];
	debug_fmt(fmt, stmt);
}

int get_reg(struct ir_operand *oper) {
	return oper->val.virt_reg;
}
char *get_ident(struct ir_operand *oper) {
	return oper->val.sym->identifier;
}
char *get_str(struct ir_operand *oper) {
	return NULL;
}
long get_cnum(struct ir_operand *oper) {
	return oper->val.constant;
}
