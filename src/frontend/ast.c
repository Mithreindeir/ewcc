#include "ast.h"

struct node *node_init(int type, void *term)
{
	struct node *n = malloc(sizeof(struct node));

	n->type = type;
	n->child = term;

	n->inf = malloc(sizeof(struct node_info));
	n->inf->type = NULL;
	n->inf->truelist = NULL, n->inf->falselist = NULL;
	n->inf->virt_reg = 0;

	return n;
}

void node_free(struct node *n)
{
	if (!n)
		return;
	switch (n->type) {
	case node_ident:
		free(IDENT(n));
		break;
	case node_cnum:	//nothing to free here
		break;
	case node_unop:
		node_free(UNOP(n)->term);
		break;
	case node_binop:
		node_free(BINOP(n)->lhs);
		node_free(BINOP(n)->rhs);
		break;
	case node_block:
		symbol_table_free(BLOCK(n)->scope);
		for (int i = 0; i < BLOCK(n)->num_stmt; i++)
			node_free(BLOCK(n)->stmt_list[i]);
		free(BLOCK(n)->stmt_list);
		break;
	case node_cond:
		node_free(COND(n)->condition);
		node_free(COND(n)->body);
		node_free(COND(n)->otherwise);
		break;
	case node_decl:
		free(DECL(n)->ident);
		node_free(DECL(n)->initializer);
		type_free(DECL(n)->type);
		break;
	case node_loop:
		node_free(LOOP(n)->init);
		node_free(LOOP(n)->condition);
		node_free(LOOP(n)->body);
		node_free(LOOP(n)->iter);
		break;
	case node_func:
		free(FUNC(n)->ident);
		type_free(FUNC(n)->ftype);
		node_free(FUNC(n)->body);
		break;
	case node_cast:
		node_free(n->child);
		n->child = NULL;/*Directly stores child*/
		break;
	case node_return:
		node_free(RET(n));
		n->child = NULL;
		break;
	}

	if (n->inf && TYPE(n))
		type_free(TYPE(n));
	free(n->inf);
	free(n->child);
	free(n);
}

struct stmt *loop_init(struct expr *init, struct expr *cond,
		       struct expr *iter, struct stmt *body)
{
	struct loop *l = malloc(sizeof(struct loop));
	l->init = init, l->condition = cond, l->iter = iter;
	l->body = body;
	return node_init(node_loop, l);
}

struct stmt *cond_init(struct expr *cond, struct stmt *body,
		       struct stmt *other)
{
	struct cond *c = malloc(sizeof(struct cond));
	c->condition = cond, c->body = body, c->otherwise = other;
	return node_init(node_cond, c);
}

struct stmt *func_init(char *ident, struct type *ftype, struct stmt *body)
{
	struct func *f = malloc(sizeof(struct func));
	f->ident = ident;
	f->ftype = ftype;
	f->body = body;
	return node_init(node_func, f);
}

struct expr *call_init(char *func, struct expr **args, int argc)
{
	struct call *c = malloc(sizeof(struct call));

	c->func = func;
	c->argv = args;
	c->argc = argc;

	return node_init(node_call, c);
}

struct stmt *block_init()
{
	struct block *b = malloc(sizeof(struct block));

	b->scope = NULL;
	b->stmt_list = NULL;
	b->num_stmt = 0;

	return node_init(node_block, b);
}

void block_addstmt(struct block *b, struct stmt *s)
{
	b->num_stmt++;
	if (!b->stmt_list)
		b->stmt_list = malloc(sizeof(struct stmt *));
	else
		b->stmt_list =
		    realloc(b->stmt_list,
			    sizeof(struct stmt *) * b->num_stmt);
	b->stmt_list[b->num_stmt - 1] = s;
}

struct expr *binop_init(enum operator   op, struct expr *lhs,
			struct expr *rhs)
{
	struct binop *b = malloc(sizeof(struct binop));
	b->lhs = lhs, b->rhs = rhs;
	b->op = op;
	return node_init(node_binop, b);
}

struct expr *unop_init(enum operator   op, struct expr *term)
{
	struct unop *u = malloc(sizeof(struct unop));
	u->term = term;
	u->op = op;
	return node_init(node_unop, u);
}

struct expr *value_ident(const char *ident)
{
	union value *v = malloc(sizeof(union value));
	int len = strlen(ident);
	v->ident = malloc(len + 1);
	memcpy(v->ident, ident, len);
	v->ident[len] = 0;
	return node_init(node_ident, v);
}

struct expr *value_num(const char *num)
{
	union value *v = malloc(sizeof(union value));
	v->cnum = strtol(num, NULL, 0);
	return node_init(node_cnum, v);
}
