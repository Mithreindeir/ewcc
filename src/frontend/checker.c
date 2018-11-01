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
		} else {
			TYPE(n) = type_copy(TYPE(UNOP(n)->term));
		}
		break;
	case node_binop:
		node_check(BINOP(n)->lhs, scope);
		node_check(BINOP(n)->rhs, scope);
		int favor = NEUTRAL;
		struct type *lt = TYPE(BINOP(n)->lhs), *rt = TYPE(BINOP(n)->rhs);
		/*Cast in binary operations goes to larger size unless it is an assignment, then favor lhs*/
		if (BINOP(n)->op == o_asn) favor = LHS;
		if (!cmp_types(lt, rt)) {
			struct binop *b = BINOP(n);
			if (!implicit_cast(&b->lhs, &b->rhs, favor)) {
				type_error(n, TYPE(BINOP(n)->lhs), TYPE(BINOP(n)->rhs));
			}
		}
		/*Pointer arithmetic check*/
		struct node *v1=NULL, *v2=NULL;
		if (BINOP(n)->lhs && lt && (lt->type == type_ptr || lt->type == type_array)) {
			v1 = BINOP(n)->lhs;
		} if (BINOP(n)->rhs && rt && (rt->type == type_ptr || rt->type == type_array)) {
			v2 = BINOP(n)->rhs;
		}
		/*The only valid operation with two pointers is subtraction*/
		if (v1 && v2 && BINOP(n)->op != o_sub) {
			printf("Invalid pointer arithmetic\n");
			node_debug(n);
			exit(-1);
		}
		if (v1 || v2) {
			if (!v1) {
				v1 = v2;
				v2 = BINOP(n)->lhs;
			} else if (!v2) {
				v2 = BINOP(n)->rhs;
			}
			/*v1 is a pointer*/
			if (BINOP(n)->op != o_add && BINOP(n)->op != o_sub) {
				printf("Invalid pointer arithmetic\n");
				node_debug(n);
				exit(-1);
			}
			/*Syntactic sugar for scaling pointer by their size*/
			//if its a constant, just multiply it
			int psize = resolve_size(TYPE(v1)->next);
			//if its a constant, just multiply it
			if (v2 && v2->type == node_cnum) {
				CNUM(v2) *= psize;
			} else if (psize != 1) {//otherwise add multiplication operation in AST
				union value *s = malloc(sizeof(union value));
				s->cnum = psize;
				struct expr *scale = binop_init(o_mul, node_init(node_cnum, s), v2);
				TYPE(scale) = type_copy(TYPE(v2));
				if (v2 == BINOP(n)->lhs) BINOP(n)->lhs = scale;
				else BINOP(n)->rhs = scale;
			}
		}
		TYPE(n) = type_copy(TYPE(BINOP(n)->lhs));
		break;
	case node_unit:
		for (int i = 0; i < UNIT(n)->num_decls; i++)
			node_check(UNIT(n)->edecls[i], scope);
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
	case node_return:
		node_check(RET(n), scope);
		break;
	case node_call:
		node_check(CALL(n)->func, scope);
		struct type *ct = TYPE(CALL(n)->func);
		while (ct && ct->type == type_ptr) ct = ct->next;
		if (ct->type != type_fcn) {
			printf("Error: called object ");
			node_debug(n);
			printf(" is not of function or function pointer type\n");
			exit(-1);
		}
		if (ct->info.fcn.paramc != CALL(n)->argc) {
			printf("Number of parameter mismatch\n");
			node_debug(n);
			exit(-1);
		}
		/*Type check parameters*/
		for (int i = 0; i < CALL(n)->argc; i++) {
			node_check(CALL(n)->argv[i], scope);
			struct type *pt = ct->info.fcn.paramt[i];
			if (!cmp_types(pt, TYPE(CALL(n)->argv[i]))) {
				if (!explicit_cast(pt, &CALL(n)->argv[i])) {
					printf("In argument %d:\n", i+1);
					type_error(n, pt, TYPE(CALL(n)->argv[i]));
				}
			}
		}
		TYPE(n) = type_copy(ct->next);
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
