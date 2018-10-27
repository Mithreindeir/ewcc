#include "peep.h"


const struct stmt_pattern pattern_stmts[NUM_PSTMT] = {
#define PATTERN(a, f, ...) __VA_ARGS__,
	PATTERN_TABLE
#undef PATTERN
};



const struct pattern peephole_patterns[NUM_PATTERNS] = {
#define PATTERN(a, func, ...) {a, &func},
	PATTERN_TABLE
#undef PATTERN
};

void optimize(struct ir_stmt *start)
{
	if (!start) return;
	while (start) {
		start = optimize_stmt(start);
		start = start->next;
	}
}

struct ir_stmt *optimize_stmt(struct ir_stmt *a)
{
	if (!a) return a;
	int offset = 0, ns = 0;
	int match = 0;
	struct ir_stmt *iter;
	struct pattern_state state = {0, NULL, NULL};

	for (int i = 0; i < NUM_PATTERNS; i++) {
		clear_state(&state);
		ns = peephole_patterns[i].num_stmt;
		iter = a;
		if (offset + ns > NUM_PSTMT) {
			break;
		}
		match = 1;
		/*first check if the statement types are right*/
		for (int j = 0; j < ns; j++) {
			if (iter->type != pattern_stmts[j+offset].st_type) {
				match = 0;
				break;
			}
			if (!match_pattern(&state, iter, pattern_stmts[j+offset])) {
				match = 0;
				break;
			}
			iter = iter->next;
			if (!iter) {
				if ((j+1) < ns)
					match = 0;
				break;
			}
		}
		if (match) {
			a = peephole_patterns[i].reduce(a);
		}

		offset += ns;
	}
	clear_state(&state);
	return a;
}

void add_state(struct pattern_state *state, void  *o, char *name)
{
	state->num_vars++;
	if (!state->vars) state->vars = malloc(sizeof(void *));
	else state->vars = realloc(state->vars, sizeof(void *) * state->num_vars);

	if (!state->names) state->names = malloc(sizeof(char*));
	else state->names = realloc(state->names, sizeof(char *) * state->num_vars);

	state->vars[state->num_vars-1] = o;
	state->names[state->num_vars-1] = name;
}

void *query_state(struct pattern_state *state, char *n)
{
	for (int i = 0; i < state->num_vars; i++)
		if (!strcmp(state->names[i], n)) return state->vars[i];
	return NULL;
}

void clear_state(struct pattern_state *state)
{
	state->num_vars = 0;
	if (state->vars) free(state->vars);
	if (state->names) free(state->names);
	state->vars = NULL;
	state->names = NULL;
}

int match_pattern(struct pattern_state *state, struct ir_stmt *a, struct stmt_pattern b)
{
	int match = 1;

	struct ir_operand *q = NULL;
	if ((*b.result)) {
		q = query_state(state, b.result);
		if (!q) add_state(state, a->result, b.result);
		if (!compare_operands(a->result, query_state(state, b.result)))
			match = 0;
	}
	if ((*b.arg1)) {
		q = query_state(state, b.arg1);
		if (!q) add_state(state, a->arg1, b.arg1);
		if (!compare_operands(a->arg1, query_state(state, b.arg1)))
			match = 0;
	}
	if ((*b.arg2)) {
		q = query_state(state, b.arg2);
		if (!q) add_state(state, a->arg2, b.arg2);
		if (!compare_operands(a->arg2, query_state(state, b.arg2)))
			match = 0;
	}
	if ((*b.label)) {
		q = query_state(state, b.label);
		if (!q) add_state(state, (void*)(uintptr_t)(a->label+1), b.label);
		if ((a->label+1) != (int)(uintptr_t)(query_state(state, b.label)))
			match = 0;
	}

	return match;
}

int compare_operands(struct ir_operand *a, struct ir_operand *b)
{
	if (!b || !a) return 0;
	if (a->type != b->type) return 0;
	enum oper_type t = a->type;
	if (t==oper_reg&&a->val.virt_reg==b->val.virt_reg) return 1;
	if (t==oper_cnum&&a->val.constant==b->val.constant) return 1;
	if (t==oper_sym&&!strcmp(a->val.ident,b->val.ident)) return 1;
	return 0;
}

struct ir_stmt *reduce_goto(struct ir_stmt *a)
{
	if (a->prev) {
		a->prev->next = a->next;
		a->next->prev = a->prev;
		struct ir_stmt *n = a->next;
		ir_stmt_free(a);
		return n;
	}
	return a;
}

struct ir_stmt *reduce_cgoto(struct ir_stmt *a)
{
	/*If the previous instruction is relational, swap that then remove the goto */
	struct ir_stmt *p = a->prev;
	struct ir_stmt *g = a->next;
	if (!g) return a;
	struct ir_stmt *gl = g->next;
	if (!gl || !p) return a;
	/*Flip the relational and change the cgoto to the goto's target*/
	if (RELATIONAL(p->type)) {
		if (p->type == stmt_lt) p->type = stmt_gte;
		else if (p->type == stmt_gt) p->type = stmt_lte;
		else if (p->type == stmt_lte) p->type = stmt_gt;
		else if (p->type == stmt_gte) p->type = stmt_lt;
		else if (p->type == stmt_eq) p->type = stmt_neq;
		else if (p->type == stmt_neq) p->type = stmt_eq;
		else return a;
	} else {
		return a;
	}

	a->label = g->label;
	g->prev->next = g->next;
	g->next->prev = g->prev;
	ir_stmt_free(g);
	return a;
}

struct ir_stmt * reduce_load(struct ir_stmt *a)
{
	struct ir_stmt *ld = a;
	struct ir_stmt *ld2 = a->next;
	ld2->prev->next = ld2->next;
	ld2->next->prev = ld2->prev;
	struct ir_stmt *cur = ld->next;
	while (cur) {
		if (compare_operands(ld2->result, cur->arg1)) {
			ir_operand_free(cur->arg1);
			cur->arg1 = copy(ld->result);
		}
		if (compare_operands(ld2->result, cur->arg2)) {
			ir_operand_free(cur->arg2);
			cur->arg2 = copy(ld->result);
		}

		cur = cur->next;
	}
	ir_stmt_free(ld2);
	return ld;
}

struct ir_stmt * reduce_store_load(struct ir_stmt *a)
{
	/*Use the value of the previous store instead of load again*/
	struct ir_stmt *ld = a->next;
	if (!ld) return a;
	/*Change the load to a move, with the previous stores operand as the value*/
	ld->type = stmt_move;
	ir_operand_free(ld->arg1);
	ld->arg1 = copy(a->arg1);
	/*Now all occurances of ld->result can be replaced with ld->arg1 */
	struct ir_stmt *cur = ld->next;
	while (cur) {
		if (compare_operands(ld->result, cur->arg1)) {
			ir_operand_free(cur->arg1);
			cur->arg1 = copy(ld->arg1);
		}
		if (compare_operands(ld->result, cur->arg2)) {
			ir_operand_free(cur->arg2);
			cur->arg2 = copy(ld->arg1);
		}

		cur = cur->next;
	}
	ld->prev->next = ld->next;
	ld->next->prev = ld->prev;
	struct ir_stmt *n = ld->next;
	ir_stmt_free(ld);
	return n;
}

struct ir_stmt *reduce_label(struct ir_stmt *a)
{
	/*If there is 2 labels, remove the 2nd and replace all occurances with the first*/
	struct ir_stmt *l2 = a->next;
	if (!l2) return a;
	int sl = l2->label;
	l2->next->prev = l2->prev;
	l2->prev->next = l2->next;

	ir_stmt_free(l2);
	struct ir_stmt *cur = a;
	/*loop forward and replace*/
	while (cur) {
		if (cur->label == sl) cur->label = a->label;
		cur = cur->next;
	}
	/*loop backwards and replace*/
	cur = a;
	while (cur) {
		if (cur->label == sl) cur->label = a->label;
		cur = cur->prev;
	}
	return a->prev;
}
