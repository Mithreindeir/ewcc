#include "debug.h"

#define STMT(a, b, c) c,
const char *stmt_debug_str[NUM_STMT] = {
	STMT_TABLE
	STMT_OPER_TABLE
};
#undef STMT

void ir_debug_fmt(const char *fmt, struct ir_stmt *stmt)
{
	struct ir_operand *cop[3] =
	    { stmt->result, stmt->arg1, stmt->arg2 };
	int len = strlen(fmt);
	for (int i = 0; i < len; i++) {
		if (fmt[i] == '$' && (i + 1) < len) {
			char c = fmt[i + 1];
			if (c >= '0' && c < '3')
				ir_operand_debug(cop[fmt[i + 1] - '0']);
			else printf("%d", stmt->label);
			i++;
		} else {
			printf("%c", fmt[i]);
		}
	}
}

void ir_operand_debug(struct ir_operand *oper)
{
	if (!oper) return;
	switch (oper->type) {
		case oper_reg: printf("R%d", oper->val.virt_reg); 	break;
		case oper_sym: printf("_%s", oper->val.ident); 		break;
		case oper_cnum: printf("#%ld", oper->val.constant); 	break;
		default: break;
	}
}

void ir_stmt_debug(struct ir_stmt *stmt)
{
	const char *fmt = stmt_debug_str[stmt->type];
	ir_debug_fmt(fmt, stmt);
}

void symbol_table_debug(struct symbol_table *scope)
{
	if (!scope)
		return;
	for (int i = 0; i < scope->num_syms; i++) {
		printf("%s: ", scope->syms[i]->identifier);
		print_type(scope->syms[i]->type);
		if ((i + 1) < scope->num_syms)
			printf(", ");
		else
			printf("\n");
	}
}

void node_debug(struct node *n)
{
	static char depth_str[256] = { 0 };
	static int di = 0;
	if (!n)
		return;
	if (di)
		printf("%.*s`--", di - 1, depth_str);
	switch (n->type) {
	case node_ident:
		printf("ident: %s\n", IDENT(n));
		break;
	case node_cnum:
		printf("cnum: %ld\n", CNUM(n));
		break;
	case node_unop:
		printf("unop \'%s\'\n", operator_str[UNOP(n)->op]);
		depth_str[di++] = ' ';
		depth_str[di++] = ' ';
		node_debug(UNOP(n)->term);
		di -= 2;
		break;
	case node_binop:
		printf("binop \'%s\':\n", operator_str[BINOP(n)->op]);
		depth_str[di++] = ' ';
		depth_str[di++] = '|';
		node_debug(BINOP(n)->lhs);
		depth_str[di - 1] = ' ';
		node_debug(BINOP(n)->rhs);
		di -= 2;
		break;
	case node_block:
		printf("block:\n");
		depth_str[di++] = ' ';
		depth_str[di++] = '|';
		for (int i = 0; i < BLOCK(n)->num_stmt; i++) {
			if ((i + 1) >= BLOCK(n)->num_stmt)
				depth_str[di - 1] = ' ';
			node_debug(BLOCK(n)->stmt_list[i]);
		}
		di -= 2;
		break;
	case node_cond:
		printf("conditional:\n");
		depth_str[di++] = '|';
		depth_str[di++] = ' ';
		node_debug(COND(n)->condition);
		if (!COND(n)->otherwise)
			depth_str[di - 2] = ' ';
		node_debug(COND(n)->body);
		depth_str[di - 2] = ' ';
		if (COND(n)->otherwise)
			node_debug(COND(n)->otherwise);
		di -= 2;;
		break;
	case node_loop:
		printf("loop:\n");
		depth_str[di++] = ' ';
		depth_str[di++] = '|';
		node_debug(LOOP(n)->init);
		node_debug(LOOP(n)->condition);
		if (!LOOP(n)->body)
			depth_str[di - 1] = ' ';
		node_debug(LOOP(n)->iter);
		depth_str[di - 1] = ' ';
		node_debug(LOOP(n)->body);
		di -= 2;
		break;
	case node_func:
		printf("func %s: ", FUNC(n)->ident);
		print_type(FUNC(n)->ftype);
		printf("\n");
		depth_str[di++] = ' ';
		depth_str[di++] = ' ';
		node_debug(FUNC(n)->body);
		di -= 2;
		break;
	case node_decl:
		printf("decl:");
		print_type(DECL(n)->type);
		printf(" %s\n", DECL(n)->ident);
		depth_str[di++] = ' ';
		depth_str[di++] = ' ';
		if (DECL(n)->initializer) {
			node_debug(DECL(n)->initializer);
		}
		di -= 2;
		break;
	case node_empty:
		printf("empty\n");
		break;
	default:
		break;
	}
}