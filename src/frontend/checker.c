#include "checker.h"

void type_error(struct node *parent, struct type *a, struct type *b)
{
	printf("Incompatable types ");
	printf("\"");
	printf("with types \"");
	print_type(a);
	printf("\" and \"");
	print_type(b);
	printf("\" in expression:\n");
	node_debug(parent);
	printf("\n");
}

int rank(struct type *a)
{
	if (!a) return 0;
	if (!a->type == type_datatype) return 0;
	enum type_specifier dt = a->info.data_type;
	if (dt==type_char) return 1;
	if (dt==type_int) return 2;
	if (dt==type_long) return 3;

	return 0;
}

int implicit_cast(struct node **a, struct node **b, int favor)
{
	struct type *ta = TYPE(*a), *tb = TYPE(*b);
	/*Integer promotion*/
	int ra = rank(ta), rb = rank(tb);
	if (ra && rb) {
		if ((ra > rb && favor == NEUTRAL) || favor == LHS) {
			//printf("Integer promotion\n");
			if (favor == LHS && rb > ra) {
				printf("Warning: Possible data loss in implicit cast from ");
				print_type(ta);
				printf(" to ");
				print_type(tb);
				printf("\n");
			}

			return explicit_cast(ta, b);
		} else if ((rb > ra && favor == NEUTRAL) || favor == RHS) {
			printf("Integer promotion\n");
			return explicit_cast(tb, a);
		}
	}
	return 0;
}

int explicit_cast(struct type *a, struct node **b)
{
	struct node *cast;
	struct type *tb = TYPE(*b);
	int ra = rank(a), rb = rank(tb);
	/*Scalar cast*/
	if (ra && rb) {
		cast = node_init(node_cast, *b);
		TYPE(cast) = type_copy(a);
		*b = cast;
		return 1;
	}
	return 0;
}

void node_check(struct node *n, struct symbol_table *scope)
{
	if (!n)
		return;
	switch (n->type) {
	case node_ident:
		TYPE(n) = type_copy(get_type(scope, IDENT(n)));
		if (!TYPE(n)) {
			printf("Use of undeclared or out of scope variables %s\n", IDENT(n));
		}
		break;
	case node_cnum:
		TYPE(n) = type_init(type_datatype);
		TYPE(n)->info.data_type = type_int;
		break;
	case node_unop:
		node_check(UNOP(n)->term, scope);
		if (UNOP(n)->op == o_ref) {
			TYPE(n) = type_init(type_ptr);
			TYPE(n)->next = type_copy(UNOP(n)->term->inf->type);
		} else if (UNOP(n)->op == o_deref) {
			/**/
			int term_t = TYPE(UNOP(n)->term)->type;
			if (term_t != type_ptr && term_t != type_array) {
				printf("Dereferencing variable not of pointer type: ");
				print_type(TYPE(UNOP(n)->term));
				printf("\n");
			}
			TYPE(n) = type_copy(TYPE(UNOP(n)->term)->next);
		}
		break;
	case node_binop:
		node_check(BINOP(n)->lhs, scope);
		node_check(BINOP(n)->rhs, scope);
		int favor = NEUTRAL;
		/*Cast in binary operations goes to larger size unless it is an assignment, then favor lhs*/
		if (BINOP(n)->op == o_asn) favor = LHS;
		if (!cmp_types(TYPE(BINOP(n)->lhs), TYPE(BINOP(n)->rhs))) {
			struct binop *b = BINOP(n);
			if (!implicit_cast(&b->lhs, &b->rhs, favor)) {
				type_error(n, TYPE(BINOP(n)->lhs), TYPE(BINOP(n)->rhs));
			}
		}
		TYPE(n) = type_copy(TYPE(BINOP(n)->lhs));
		break;
	case node_block:
		for (int i = 0; i < BLOCK(n)->num_stmt; i++) {
			node_check(BLOCK(n)->stmt_list[i],
				   BLOCK(n)->scope);
		}
		break;
	case node_func:
		  node_check(FUNC(n)->body, scope);
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
		if (!DECL(n)->initializer)
			break;
		struct type *dtype = get_type(scope, DECL(n)->ident);
		if (!cmp_types(dtype, TYPE(DECL(n)->initializer))) {
			if (!explicit_cast(dtype, &DECL(n)->initializer)) {
				type_error(n, dtype, TYPE(DECL(n)->initializer));
			}
		}
		break;
	default:
		break;
	}
}
