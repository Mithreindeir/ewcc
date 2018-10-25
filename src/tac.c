#include "tac.h"

#define STMT(a, b, c) c,
const char *stmt_str[NUM_STMT] = {
	STMT_TABLE
	STMT_OPER_TABLE
};

#undef STMT

enum stmt_type map_stmt(enum operator op)
{
#define STMT(a, b, c) case b: return STMT_STR(a);
	switch (op) {
		STMT_OPER_TABLE
		default: break;
	}
#undef STMT
	return stmt_label;
}

struct ir_stmt *generate(struct node *top)
{
	struct generator context = { .reg_cnt = 0, .scope = NULL};
	generate_from_node(top, &context);
	return context.head;
}

void generate_from_node(struct node *n, struct generator *context)
{
	if (!n) return;
	struct ir_stmt *stmt = ir_stmt_init();
	switch (n->type) {
	case node_unop:/*Unary operations*/
		stmt->arg1 = 	generate_operand(UNOP(n)->term, context, VALUE);
		stmt->result = 	from_reg(context->reg_cnt++);
		stmt->type = map_stmt(BINOP(n)->op);
		break;
	case node_binop: /*If the type is assign, get the address of the lhs and assign the rhs to it*/
		stmt->type = map_stmt(BINOP(n)->op);
		if (stmt->type == stmt_store) {
			stmt->arg1 = 	generate_operand(BINOP(n)->rhs, context, VALUE);
			stmt->result = 	generate_operand(BINOP(n)->lhs, context, ADDRESS);
		} else {
			stmt->arg1 = 	generate_operand(BINOP(n)->lhs, context, VALUE);
			stmt->arg2 = 	generate_operand(BINOP(n)->rhs, context, VALUE);
			stmt->result = 	from_reg(context->reg_cnt++);
		}
		break;
	case node_block:
		for (int i = 0; i < BLOCK(n)->num_stmt; i++)
			generate_from_node(BLOCK(n)->stmt_list[i], context);
		break;
	case node_cond:;
		/*Save Context start*/
		int otherwise = REQ_LABEL(context), end = REQ_LABEL(context);
		generate_operand(COND(n)->condition, context, IGNORE);
		emit_jump(context, otherwise, CONDITIONAL);
		generate_from_node(COND(n)->body, context);
		emit_jump(context, otherwise, UNCONDITIONAL);
		emit_label(context, otherwise);
		generate_from_node(COND(n)->otherwise, context);
		emit_label(context, end);
		break;
	case node_decl: /*emit an alloca, and optional assign statement*/
		stmt->type = stmt_alloc;
		stmt->arg1 = from_ident(DECL(n)->ident);
		if (DECL(n)->initializer) {
			emit(context, stmt);
			stmt = ir_stmt_init();
			stmt->type = stmt_store;
			stmt->arg1 = generate_operand(DECL(n)->initializer, context, VALUE);
			stmt->result = from_ident(DECL(n)->ident);
		}
		break;
	case node_loop:
		break;
	case node_func:
		break;
	default: /**/
		printf("Corruped AST\n");
		break;
	}
	if (stmt->type == stmt_invalid) {
		free(stmt);
		return;
	}
	emit(context, stmt);
}

struct ir_operand *generate_operand(struct node *n, struct generator *context, int result)
{
	if (!n) return NULL;
	struct ir_stmt *stmt = ir_stmt_init();
	switch (n->type) {
	case node_ident: /*Load ident into a register*/
		if (result == ADDRESS) {
			free(stmt);
			return from_ident(IDENT(n));
		}
		stmt->type = stmt_load;
		stmt->result = from_reg(context->reg_cnt++);
		stmt->arg1 = from_ident(IDENT(n));
		emit(context, stmt);
		return copy(stmt->result);
	case node_cnum:
		free(stmt);
		return from_cnum(CNUM(n));
	case node_unop:
		stmt->arg1 = 	generate_operand(UNOP(n)->term, context, VALUE);
		stmt->result = 	from_reg(context->reg_cnt++);
		stmt->type = map_stmt(BINOP(n)->op);
		emit(context, stmt);
		return copy(stmt->result);
	case node_binop:
		stmt->arg1 = 	generate_operand(BINOP(n)->lhs, context, VALUE);
		stmt->arg2 = 	generate_operand(BINOP(n)->rhs, context, VALUE);
		stmt->type = map_stmt(BINOP(n)->op);
		emit(context, stmt);
		if (result == IGNORE) return NULL;
		stmt->result = 	from_reg(context->reg_cnt++);
		return copy(stmt->result);
	default: /**/
		printf("Corrupted AST\n");
		break;
	}
	free(stmt);
	return NULL;
}

struct ir_stmt *emit_label(struct generator *context, int label)
{
	struct ir_stmt *stmt = ir_stmt_init();
	stmt->type = stmt_label;
	stmt->label = label;
	emit(context, stmt);
	return stmt;
}

struct ir_stmt *emit_jump(struct generator *context, int label, int conditional)
{
	struct ir_stmt *stmt = ir_stmt_init();
	if (conditional == CONDITIONAL)
		stmt->type = stmt_cgoto;
	else
		stmt->type = stmt_ugoto;
	stmt->label = label;
	emit(context, stmt);
	return stmt;
}

void emit(struct generator *context, struct ir_stmt *stmt)
{
	if (!context->head) {
		context->head = stmt;
		context->cur = stmt;
	} else {
		context->cur->next = stmt;
		stmt->prev = context->cur;
		context->cur = stmt;
	}
}

struct ir_stmt *ir_stmt_init()
{
	struct ir_stmt *stmt = malloc(sizeof(struct ir_stmt));
	stmt->type=stmt_invalid;
	stmt->result = NULL, stmt->arg1 = NULL, stmt->arg2 = NULL;
	stmt->prev = NULL, stmt->next = NULL;
	return stmt;
}

struct ir_operand *copy(struct ir_operand *oper)
{
	if (oper->type == oper_reg)
		return from_reg(oper->val.virt_reg);
	else if (oper->type == oper_sym)
		return from_ident(oper->val.ident);
	return NULL;
}

struct ir_operand *from_reg(int reg)
{
	struct ir_operand *oper = malloc(sizeof(struct ir_operand));
	oper->type = oper_reg;
	oper->val.virt_reg = reg;
	return oper;
}

struct ir_operand *from_ident(char *ident)
{
	struct ir_operand *oper = malloc(sizeof(struct ir_operand));
	oper->type = oper_sym;
	oper->val.ident = ident;
	return oper;
}

struct ir_operand *from_cnum(long cnum)
{
	struct ir_operand *oper = malloc(sizeof(struct ir_operand));
	oper->type = oper_cnum;
	oper->val.constant = cnum;
	return oper;
}

void ir_operand_free(struct ir_operand *oper)
{
	if (!oper) return;
	free(oper);
}

void ir_stmt_free(struct ir_stmt *stmt)
{
	if (!stmt) return;
	ir_operand_free(stmt->result);
	ir_operand_free(stmt->arg1);
	ir_operand_free(stmt->arg2);
	free(stmt);
}
