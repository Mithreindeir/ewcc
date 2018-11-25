#include "debug.h"

#define STMT(a, b, c) c,
const char *stmt_debug_str[NUM_STMT] = {
	STMT_TABLE STMT_OPER_TABLE
};

#undef STMT

void bb_debug(struct bb *bb)
{
	if (!bb)
		return;
	bb->visited = 1;
	//printf("Basic Block: (%d-%d)\n", bb->ln, bb->ln+bb->len-1);
	//printf("------------------------------------\n");
	for (int i = 0; i < bb->nphi; i++) {
		struct phi p = bb->phi_hdr[i];
		struct symbol *sym = p.var;
		//if (sym->offset)
		//	printf("[fp-%ld]", sym->offset);
		printf("%s", sym->identifier);
		if (p.piter)
			printf("_%d", p.piter);
		printf(" = (");
		for (int j = 0; j < p.niter; j++) {
			printf("%d", p.iters[j]);
			if ((j+1) < p.niter)
				printf(", ");
		}
		printf(")\n");
	}

	printf("IN: ");
	for (int i = 0; i < bb->nin; i++) {
		ir_operand_debug(bb->in[i]);
		if ((i+1) < bb->nin) printf(", ");
	}
	printf("\n");
	struct ir_stmt *stmt = bb->blk;
	int ln = 0;
	while (stmt && ln < bb->len) {
		printf("%2d: ", ++ln + bb->ln);
		ir_stmt_debug(stmt);
		printf("\n");
		stmt = stmt->next;
	}
	printf("OUT: ");
	for (int i = 0; i < bb->nout; i++) {
		ir_operand_debug(bb->out[i]);
		if ((i+1) < bb->nout) printf(", ");
	}
	printf("\n");
	return;
	for (int i = 0; i < bb->nsucc; i++) {
		bb_debug(bb->succ[i]);
	}
}

void ir_debug_fmt(const char *fmt, struct ir_stmt *stmt)
{
	struct ir_operand *cop[3] =
	    { stmt->result, stmt->arg1, stmt->arg2 };
	int len = strlen(fmt);
	if (stmt->type != stmt_label && stmt->type != stmt_func)
		printf("\t");
	for (int i = 0; i < len; i++) {
		if (fmt[i] == '$' && (i + 1) < len) {
			char c = fmt[i + 1];
			if (c >= '0' && c < '3')
				ir_operand_debug(cop[fmt[i + 1] - '0']);
			else if (c == 'l')
				printf("%d", stmt->label);
			i++;
		} else if (fmt[i] == '@' && (i + 1) < len) {
			char c = fmt[i + 1];
			if (c >= '0' && c < '3')
				ir_operand_size_debug(cop
						      [fmt[i + 1] - '0']);
			i++;
		} else {
			printf("%c", fmt[i]);
		}
	}
	if (stmt->type == stmt_func) printf(":");
	return;
	ir_operand_size_debug(cop[0]);
	ir_operand_size_debug(cop[1]);
	ir_operand_size_debug(cop[2]);
}

void ir_operand_debug(struct ir_operand *oper)
{
	if (!oper)
		return;
	switch (oper->type) {
	case oper_reg:
		printf("r%d", oper->val.virt_reg);
		break;
	case oper_sym:
		if (!oper->val.sym)
			break;
		if (oper->val.sym->offset)
			printf("fp-%ld", oper->val.sym->offset);
		else
			printf("_%s", oper->val.sym->identifier);
		if (oper->iter)
			printf("_%d", oper->iter);
		break;
	case oper_cnum:
		printf("#%ld", oper->val.constant);
		break;
	default:
		break;
	}
}

void ir_operand_size_debug(struct ir_operand *oper)
{
	if (!oper)
		return;
	/*print size */
	if (oper->size == 0)
		printf("n");
	else if (oper->size == 1)
		printf("b");
	else if (oper->size == 2)
		printf("w");
	else if (oper->size == 4)
		printf("dw");
	else if (oper->size == 8)
		printf("qw");
	else
		printf("[%d]", oper->size);
}

void ir_stmt_debug(struct ir_stmt *stmt)
{
	const char *fmt = stmt_debug_str[stmt->type];
	ir_debug_fmt(fmt, stmt);
}

void ir_debug(struct ir_stmt *head)
{
	struct ir_stmt *stmt = head;
	int ln = 0;
	while (stmt) {
		printf("%2d: ", ++ln);
		ir_stmt_debug(stmt);
		printf("\n");
		stmt = stmt->next;
	}
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
	if (di > 250) {
		di = 0;
	}
	if (!n)
		return;
	if (di)
		printf("%.*s`--", di - 1, depth_str);
	//printf("%.*s`--(%d)", di - 1, depth_str, TYPE(n) ? resolve_size(TYPE(n)) : 0);
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
	case node_unit:
		for (int i = 0; i < UNIT(n)->num_decls; i++)
			node_debug(UNIT(n)->edecls[i]);
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
	case node_call:
		printf("call\n");
		depth_str[di++] = ' ';
		depth_str[di++] = '|';
		node_debug(CALL(n)->func);
		for (int i = 0; i < CALL(n)->argc; i++) {
			if ((i + 1) >= CALL(n)->argc)
				depth_str[di - 1] = ' ';
			node_debug(CALL(n)->argv[i]);
		}
		depth_str[di - 1] = ' ';
		di -= 2;

		break;
	case node_decl_list:
		for (int i = 0; i < DECL_L(n)->num_decls; i++) {
			if (i != 0)
				printf("%.*s`--", di - 1, depth_str);
			struct declaration *d = DECL_L(n)->decls[i];
			printf("decl:");
			depth_str[di++] = ' ';
			if ((i+1)<DECL_L(n)->num_decls)
				depth_str[di++] = '|';
			else depth_str[di++] = ' ';
			print_type(d->type);
			printf(" %s\n", d->ident);
			if (d->initializer) {
				node_debug(d->initializer);
			}
			di -= 2;
		}
		break;
	case node_empty:
		printf("empty\n");
		break;
	case node_break:
		printf("break\n");
		break;
	case node_continue:
		printf("continue\n");
		break;
	case node_return:
		printf("return\n");
		depth_str[di++] = ' ';
		depth_str[di++] = ' ';
		node_debug(RET(n));
		di -= 2;
		break;
	case node_cast:
		printf("cast: ");
		print_type(TYPE(n));
		printf("\n");
		depth_str[di++] = ' ';
		depth_str[di++] = ' ';
		node_debug(n->child);
		di -= 2;
		break;
	default:
		break;
	}
}
