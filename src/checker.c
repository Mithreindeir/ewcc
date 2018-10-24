#include "checker.h"

void type_error(struct node *parent, struct type*a, struct type *b)
{
	printf("Incompatable types ");
	printf("\"");
	printf("with types \"");
	print_type(a);
	printf("\" and \"");
	print_type(b);
	printf("\" in expression \"");
	node_print(parent);
	printf("\"\n");
}

void node_check(struct node *n, struct symbol_table *scope)
{
	return;
	if (!n) return;
	switch (n->type) {
		case node_ident://deliberate fallthrough
			n->inf->type = get_type(scope, IDENT(n));
			if (!n->inf->type) {
				printf("Use of undeclared or out of scope variables %s\n", IDENT(n));
			}
			break;
		case node_cnum:
			n->inf->type = type_init(type_datatype);
			n->inf->type->info.data_type = type_int;
			break;
		case node_unop:
			node_check(UNOP(n)->term, scope);
			if (UNOP(n)->op == o_ref) {
				n->inf->type = type_init(type_ptr);
				n->inf->type->next = UNOP(n)->term->inf->type;
			} else if (UNOP(n)->op == o_deref) {
				n->inf->type = UNOP(n)->term->inf->type->next;
			}
			break;
		case node_binop:
			node_check(BINOP(n)->lhs, scope);
			node_check(BINOP(n)->rhs, scope);
			if (!cmp_types(BINOP(n)->lhs->inf->type, BINOP(n)->rhs->inf->type)) {
				type_error(n, BINOP(n)->lhs->inf->type, BINOP(n)->rhs->inf->type);
			}
			n->inf->type = BINOP(n)->lhs->inf->type;
			break;
		case node_block:
			for (int i = 0; i < BLOCK(n)->num_stmt; i++) {
				node_check(BLOCK(n)->stmt_list[i], BLOCK(n)->scope);
			}
			break;
		case node_cond:
			node_check(COND(n)->condition, scope);
			node_check(COND(n)->body, scope);
			if (COND(n)->otherwise) {
				node_check(COND(n)->otherwise, scope);
			}
			break;
		case node_loop:
			node_check(LOOP(n)->init, scope);
			node_check(LOOP(n)->condition, scope);
			node_check(LOOP(n)->body, scope);
			node_check(LOOP(n)->iter, scope);
			break;
		case node_decl:
			node_check(DECL(n)->initializer, scope);
			if (!DECL(n)->initializer) break;
			if (!cmp_types(get_type(scope, DECL(n)->ident), DECL(n)->initializer->inf->type)) {
				type_error(n, get_type(scope, DECL(n)->ident), DECL(n)->initializer->inf->type);
			}
			break;
		default: break;
	}
}
