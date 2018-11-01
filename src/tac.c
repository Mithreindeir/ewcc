#include "tac.h"

/*Not a 1-1 mapping, so generate jump table instead of mapping array*/
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
	case node_block: 	block_emit(context, BLOCK(n)); break;
	case node_cond: 	cond_emit(context, COND(n)); break;
	case node_decl: 	decl_emit(context, DECL(n)); break;
	case node_loop:		loop_emit(context, LOOP(n)); break;
	case node_cast:		r = generate_from_node(n->child, context, result); break;
	case node_call: 	r = call_emit(context, CALL(n), result); break;
	case node_ident:    	r = ident_emit(context, IDENT(n), result); break;
	case node_cnum:		r = from_cnum(CNUM(n)); break;
	case node_unop:
		r = unop_emit(context, UNOP(n), result);
		/*The final value gets the type of the unary expression*/
		if (r &&& TYPE(n)) r->size = resolve_size(TYPE(n));
		break;
	case node_binop:
		r = binop_emit(context, BINOP(n), result);
		/*The final value gets the type of the binary expression*/
		if (r &&& TYPE(n)) r->size = resolve_size(TYPE(n));
		break;
	case node_unit:
		for (int i = 0; i < UNIT(n)->num_decls; i++)
			generate_from_node(UNIT(n)->edecls[i], context, IGNORE);
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
	case node_empty: break;
	default:
		printf("Unknown AST Node %d\n", n->type);
		exit(-1);
		break;
	}
	context->parent = last;
	return r;
}

void block_emit(struct generator *context, struct block *b)
{
	struct node *p = context->parent;
	context->scope = b->scope;
	for (int i = 0; i < b->num_stmt; i++) {
		generate_from_node(b->stmt_list[i], context, IGNORE);
		/*Merge it into the block*/
		TLIST(p)=merge(TLIST(p), TLIST(b->stmt_list[i]));
		FLIST(p)=merge(FLIST(p), FLIST(b->stmt_list[i]));
	}
	context->scope = b->scope->parent;
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

/*Emits jump to comparison with jumps at the end to the body to continue the loop*/
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

/*For now, allocate appropriate size after frame pointer, and set the symbols value*/
void decl_emit(struct generator *context, struct declaration *decl)
{
	struct ir_stmt *stmt = ir_stmt_init();
	stmt->type = stmt_alloc;
	alloc_type(context->scope, decl->ident);
	struct symbol *s = get_symbol(context->scope, decl->ident);
	stmt->arg1 = from_sym(s);
	if (decl->initializer) {
		emit(context, stmt);
		stmt = ir_stmt_init();
		stmt->type = stmt_store;
		stmt->arg2 = generate_from_node(decl->initializer, context, VALUE);
		stmt->arg1 = from_sym(s);
	}
	emit(context, stmt);
}

/*Generates statments to set the params to the args, then inserts a call, optionally saving result*/
struct ir_operand *call_emit(struct generator *context, struct call *c, int result)
{
	struct ir_stmt *s = ir_stmt_init();
	struct type *ct = TYPE(context->parent);
	/*If function has return type, then store result in register*/
	if (ct && ct->type == type_datatype) {
		if (ct->info.data_type != type_void) {
			s->result = from_reg(context->reg_cnt++);
		}
	}
	for (int i = c->argc-1; i >= 0; i--) {
		struct ir_stmt *p = ir_stmt_init();
		p->type = stmt_param;
		p->arg1 = from_cnum(i);
		p->arg2 = generate_from_node(c->argv[i], context, VALUE);
		emit(context, p);
	}
	s->type = stmt_call;
	s->arg1 = generate_from_node(c->func, context, ADDRESS);
	emit(context, s);
	return result != IGNORE ? copy(s->result) : NULL;
}

/*If the result is asked for, returns either the address or value paired with a symbol*/
struct ir_operand *ident_emit(struct generator *context, char *ident, int result)
{
	//if (result == IGNORE) return NULL;
	struct symbol *s = get_symbol(context->scope, ident);
	if (result == ADDRESS) return from_sym(s);
	/*Arrays are converted to pointers to the first member of the array*/
	if (TYPE(context->parent) && TYPE(context->parent)->type == type_array) {
		struct ir_stmt *stmt = ir_stmt_init();
		stmt->type = stmt_addr;
		stmt->result = from_reg(context->reg_cnt++);
		stmt->arg1 = from_sym(s);
		emit(context, stmt);
		return copy(stmt->result);
	}
	/*If the value is asked for then emit a load instruction with the symbol address*/
	struct ir_stmt *stmt = ir_stmt_init();
	stmt->type = stmt_load;
	stmt->result = from_reg(context->reg_cnt++);
	stmt->arg1 = from_sym(s);
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
		stmt->arg1 = generate_from_node(u->term, context, ADDRESS);
		stmt->arg2 = lreg;
		emit(context, stmt);
		/*Post inc/dec returns the old value in a register, then increments
		 * contents at the address*/
		if (op==o_postinc || op==o_postdec)
			return result != IGNORE ? copy(preg) : NULL;
		else /*Pre inc/dec increments it first then returns the final register*/
			return result != IGNORE ? copy(lreg) : NULL;
	} else if (stmt->type == stmt_load &&  TYPE(u->term)->type == type_array) {
		/*Arrays are not pointers to pointers to arrays, just pointers to arrays
		 * So remove the first dereference if storing in an array*/
		if (result==ADDRESS) stmt->type = stmt_move;
		stmt->arg1 = generate_from_node(u->term, context, ADDRESS);
	} else if (stmt->type == stmt_load && result == ADDRESS) {
		//Do not dereference, if an address is needed as a result
		free(stmt);
		return generate_from_node(u->term, context, VALUE);
	} else if (stmt->type == stmt_load) {
		/*If there is a deref/ref involved, let the parent decide*/
		stmt->arg1 = generate_from_node(u->term, context, ADDRESS);
	} else if (stmt->type == stmt_addr) {
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
		/*Uses arg1 instead of result because the write is a side effect*/
		stmt->arg2 = 	generate_from_node(b->rhs, context, VALUE);
		stmt->arg1 = generate_from_node(b->lhs, context, ADDRESS);
		emit(context, stmt);
		//return result != IGNORE ? copy(stmt->arg1) : NULL;
		return result != IGNORE ? (result == ADDRESS ? copy(stmt->arg1) : copy(stmt->arg2)) : NULL;
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
		stmt->arg1 = generate_from_node(b->lhs, context, VALUE);
		stmt->arg2 = generate_from_node(b->rhs, context, VALUE);
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

void eval_size(struct ir_stmt *stmt)
{
	int dfsize = 0;
	if (stmt->result && stmt->result->size) dfsize = stmt->result->size;
	else if (stmt->arg1 && stmt->arg1->size) dfsize = stmt->arg1->size;
	else if (stmt->arg2 && stmt->arg2->size) dfsize = stmt->arg2->size;

	if (stmt->result && !stmt->result->size)
		stmt->result->size = dfsize;
	if (stmt->arg1 && !stmt->arg1->size)
		stmt->arg1->size = dfsize;
	if (stmt->arg2 && !stmt->arg2->size)
		stmt->arg2->size = dfsize;
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
	eval_size(stmt);
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
	struct ir_operand *cpy = NULL;
	if (oper->type == oper_reg)
		cpy = from_reg(oper->val.virt_reg);
	else if (oper->type == oper_sym)
		cpy = from_sym(oper->val.sym);
	else if (oper->type == oper_cnum)
		cpy = from_cnum(oper->val.constant);
	if (cpy) cpy->size = oper->size;
	return cpy;
}

struct ir_operand *from_reg(int reg)
{
	struct ir_operand *oper = malloc(sizeof(struct ir_operand));
	oper->size = 0;
	oper->type = oper_reg;
	oper->val.virt_reg = reg;
	return oper;
}

struct ir_operand *from_sym(struct symbol *s)
{
	if (!s) {
		printf("Unresolved symbol!\n");
		exit (-1);
	}
	struct ir_operand *oper = malloc(sizeof(struct ir_operand));
	oper->size = s->size;
	oper->type = oper_sym;
	oper->val.sym = s;
	return oper;
}

struct ir_operand *from_cnum(long cnum)
{
	struct ir_operand *oper = malloc(sizeof(struct ir_operand));
	oper->size = INT_TYPE_SIZE;//Default constant size is integer
	oper->type = oper_cnum;
	oper->val.constant = cnum;
	return oper;
}

/*The values are borrowed from the AST*/
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
