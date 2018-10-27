#include "tac.h"

enum stmt_type map_stmt(enum operator op)
{
	switch (op) {
#define STMT(a, b, c) case b: return STMT_STR(a);
		STMT_OPER_TABLE
#undef STMT
		default: break;
	}
	return stmt_invalid;
}

struct ir_stmt *generate(struct node *top)
{
	struct generator context = { .reg_cnt = 0, .scope = NULL, .parent = top};
	generate_from_node(top, &context, IGNORE);
	return context.head;
}

struct ir_operand *generate_from_node(struct node *n, struct generator *context, int result)
{
	if (!n) return NULL;
	struct ir_operand *r = NULL;
	struct node *last = context->parent;
	context->parent = n;
	switch (n->type) {
	case node_ident:
		r = ident_emit(context, IDENT(n), result);
		break;
	case node_cnum:
		r = from_cnum(CNUM(n));
		break;
	case node_unop:/*Unary operations*/
		r = unop_emit(context, UNOP(n), result);
		break;
	case node_binop: /*If the type is assign, get the address of the lhs and assign the rhs to it*/
		r = binop_emit(context, BINOP(n), result);
		break;
	case node_block:
		for (int i = 0; i < BLOCK(n)->num_stmt; i++) {
			generate_from_node(BLOCK(n)->stmt_list[i], context, IGNORE);
			/*Merge it into the block*/
			TLIST(n)=merge(TLIST(n), TLIST(BLOCK(n)->stmt_list[i]));
			FLIST(n)=merge(FLIST(n), FLIST(BLOCK(n)->stmt_list[i]));
		}
		break;
	case node_cond:
		cond_emit(context, COND(n));
		break;
	case node_decl: /*emit an alloca, and optional assign statement*/
		decl_emit(context, DECL(n));
		break;
	case node_loop:
		loop_emit(context, LOOP(n));
		break;
	case node_func:
		generate_from_node(FUNC(n)->body, context, IGNORE);
		int end = REQ_LABEL(context);
		emit_label(context, end);
		backpatch(FLIST_REF(FUNC(n)->body), end);
		struct ir_stmt *s = ir_stmt_init();
		s->type = stmt_ret;
		emit(context, s);
		break;
	case node_break:
		FLIST(n) = make_list(emit_jump(context, -1, UNCONDITIONAL));
		break;
	case node_continue:
		TLIST(n) = make_list(emit_jump(context, -1, UNCONDITIONAL));
		break;
	case node_return:
		s = ir_stmt_init();
		s->type = stmt_retval;
		s->arg1 = generate_from_node(RET(n), context, VALUE);
		emit(context, s);
		FLIST(n) = make_list(emit_jump(context, -1, UNCONDITIONAL));
		break;
	default: exit(-1); break;
	}
	context->parent = last;
	return r;
}

void cond_emit(struct generator *context, struct cond *c)
{
	/*Emit comparison, if false jump to block end, at end of block jump after else block*/
	int start = REQ_LABEL(context), otherwise = REQ_LABEL(context), end = REQ_LABEL(context);
	generate_from_node(c->condition, context, IGNORE);
	/*True is patched to the start of the block, false is patched to where the else block would be*/
	backpatch(TLIST_REF(c->condition), start);
	backpatch(FLIST_REF(c->condition), otherwise);

	emit_label(context, start);
	generate_from_node(c->body, context, IGNORE);
	/*If there is a else block, then emit an unconditional jump to the end of that*/
	if (c->otherwise)
		emit_jump(context, end, UNCONDITIONAL);
	emit_label(context, otherwise);
	/*If else statement exists, then traverse that too*/
	generate_from_node(c->otherwise, context, IGNORE);
	emit_label(context, end);
	/*The body's patched to the parents*/
	if (c->body) {
		TLIST(context->parent) = merge(TLIST(context->parent), TLIST(c->body));
		FLIST(context->parent) = merge(FLIST(context->parent), FLIST(c->body));
	}
	if (c->otherwise) {
		TLIST(context->parent) = merge(TLIST(context->parent), TLIST(c->otherwise));
		FLIST(context->parent) = merge(FLIST(context->parent), FLIST(c->otherwise));
	}
}

void loop_emit(struct generator *context, struct loop *l)
{
	int cmp = REQ_LABEL(context), start = REQ_LABEL(context), end = REQ_LABEL(context);
	generate_from_node(l->init, context, IGNORE);
	/*Unconditional jump to compare block*/
	emit_jump(context, cmp, UNCONDITIONAL);
	emit_label(context, start);
	generate_from_node(l->body, context, IGNORE);
	generate_from_node(l->iter, context, IGNORE);
	emit_label(context, cmp);
	generate_from_node(l->condition, context, IGNORE);
	emit_label(context, end);
	/*True is backpatched to the loop start, false is patched to after the loop ends*/
	backpatch(TLIST_REF(l->condition), start);
	backpatch(FLIST_REF(l->condition), end);
	/*The bodys truelist/falselist are continue/break, so patch to start/end respectively*/
	backpatch(TLIST_REF(l->body), start);
	backpatch(FLIST_REF(l->body), end);
	/*The loop body gets patched up to the parent*/
	if (l->body) {
		TLIST(context->parent) = merge(TLIST(context->parent), TLIST(l->body));
		FLIST(context->parent) = merge(FLIST(context->parent), FLIST(l->body));
	}
}

void decl_emit(struct generator *context, struct declaration *decl)
{
	struct ir_stmt *stmt = ir_stmt_init();
	stmt->type = stmt_alloc;
	stmt->arg1 = from_ident(decl->ident);
	if (decl->initializer) {
		emit(context, stmt);
		stmt = ir_stmt_init();
		stmt->type = stmt_store;
		stmt->arg1 = generate_from_node(decl->initializer, context, VALUE);
		stmt->result = from_ident(decl->ident);
	}
	emit(context, stmt);
}

/*If the result is asked for, returns either the address or value paired with a symbol*/
struct ir_operand *ident_emit(struct generator *context, char *ident, int result)
{
	//if (result == IGNORE) return NULL;
	if (result == ADDRESS) return from_ident(ident);
	/*If the value is asked for then emit a load instruction with the symbol address*/
	struct ir_stmt *stmt = ir_stmt_init();
	stmt->type = stmt_load;
	stmt->result = from_reg(context->reg_cnt++);
	stmt->arg1 = from_ident(ident);
	emit(context, stmt);
	return copy(stmt->result);
}

/*Traverses the unary expression and emits IR code, handles ++/-- syntactic sugar
 * and returns the result if asked*/
struct ir_operand *unop_emit(struct generator *context, struct unop *u, int result)
{
	struct ir_stmt *stmt = ir_stmt_init();
	enum operator op = u->op;
	stmt->type = map_stmt(op);
	if (op==o_preinc || op == o_predec || op == o_postinc || op == o_postdec ) {
		stmt->type = (op==o_preinc||op==o_postinc) ? stmt_add : stmt_sub;
		stmt->arg1 = 	generate_from_node(u->term, context, VALUE);
		stmt->arg2 = 	from_cnum(1);
		stmt->result = 	from_reg(context->reg_cnt++);
		emit(context, stmt);
		struct ir_operand *lreg = copy(stmt->result), *preg = stmt->arg1;
		stmt = ir_stmt_init();
		stmt->type = stmt_store;
		stmt->result = generate_from_node(u->term, context, ADDRESS);
		stmt->arg1 = lreg;
		emit(context, stmt);
		/*Post inc/dec returns the old value in a register, then increments
		 * contents at the address*/
		if (op==o_postinc || op==o_postdec)
			return result != IGNORE ? copy(preg) : NULL;
		else /*Pre inc/dec increments it first then returns the final register*/
			return result != IGNORE ? copy(lreg) : NULL;
	} else if (stmt->type == stmt_addr || stmt->type == stmt_load) {
		/*Ref statements or loads get the address of the operand*/
		stmt->arg1 = generate_from_node(u->term, context, ADDRESS);
	} else {
		/*Most instructions use the value of the operands*/
		stmt->arg1 = generate_from_node(u->term, context, VALUE);
	}
	stmt->result = 	from_reg(context->reg_cnt++);
	emit(context, stmt);
	return result != IGNORE ? copy(stmt->result) : NULL;
}

/*Traverses the binary expression and emits IR code, handling short circuit and booleans
 * returns the value of the operation if the result is asked for*/
struct ir_operand *binop_emit(struct generator *context, struct binop *b, int result)
{
	struct ir_stmt *stmt = ir_stmt_init();
	enum operator op = b->op;
	stmt->type = map_stmt(op);
	/*Store dereferences an address, and stores the rhs value in it*/
	if (stmt->type == stmt_store) {
		stmt->arg1 = 	generate_from_node(b->rhs, context, VALUE);
		stmt->result = generate_from_node(b->lhs, context, ADDRESS);
	} else if (op == o_and) {
		/*And short circuit only evaluates the first expression if it is false*/
		int between = REQ_LABEL(context);
		generate_from_node(b->lhs, context, IGNORE);
		emit_label(context, between);
		generate_from_node(b->rhs, context, IGNORE);
		/*Patch the truelist to the next expression*/
		backpatch(TLIST_REF(b->lhs), between);
		/*The truelist becomes the next expressions truelist*/
		TLIST(context->parent) = TLIST(b->rhs);
		/*Both sides falselists point to the same result*/
		FLIST(context->parent) = merge(FLIST(b->lhs), FLIST(b->rhs));
		free(stmt);
		return NULL;
	} else if (RELATIONAL(stmt->type)) {
		/*Relational operators are actually jumps*/
		stmt->arg1 = generate_from_node(b->lhs, context, IGNORE);
		stmt->arg2 = generate_from_node(b->rhs, context, IGNORE);
		emit(context, stmt);
		/*The conditionally jumps if the expression is false*/
		TLIST(context->parent) = make_list(emit_jump(context, -1, CONDITIONAL));
		/*Unconditional jump to the next statement (will be optimized out)*/
		FLIST(context->parent) = make_list(emit_jump(context, -1, UNCONDITIONAL));
		return NULL;
	} else {
		stmt->arg1 = generate_from_node(b->lhs, context, VALUE);
		stmt->arg2 = generate_from_node(b->rhs, context, VALUE);
		stmt->result = from_reg(context->reg_cnt++);
	}
	emit(context, stmt);
	return result != IGNORE ? copy(stmt->result) : NULL;
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
	stmt->label = -1;
	return stmt;
}

struct ir_operand *copy(struct ir_operand *oper)
{
	if (oper->type == oper_reg)
		return from_reg(oper->val.virt_reg);
	else if (oper->type == oper_sym)
		return from_ident(oper->val.ident);
	else if (oper->type == oper_cnum)
		return from_cnum(oper->val.constant);
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

struct ir_list *make_list(struct ir_stmt *n)
{
	struct ir_list *list = malloc(sizeof(struct ir_list));
	list->list = malloc(sizeof(struct ir_stmt*));
	list->list[0] = n;
	list->num_stmt = 1;
	return list;
}

void free_list(struct ir_list *list)
{
	free(list->list);
	free(list);
}

struct ir_list *merge(struct ir_list *list1, struct ir_list *list2)
{
	if (!list1 && list2) return list2;
	if (!list2 && list1) return list1;
	if (!list1 && !list2) return NULL;

	int os = list1->num_stmt;
	int ns = list1->num_stmt + list2->num_stmt;
	list1->num_stmt = ns;
	list1->list = realloc(list1->list, sizeof(struct ir_stmt*)*ns);
	for (int i = os; i < ns; i++) {
		list1->list[i] = list2->list[i-os];
	}
	free_list(list2);
	return list1;
}

void backpatch(struct ir_list **rlist, int label)
{
	if (!rlist) return;
	struct ir_list *list = *rlist;
	if (!list) return;
	for (int i = 0; i < list->num_stmt; i++)
		list->list[i]->label = label;
	free_list(list);
	*rlist = NULL;
}
