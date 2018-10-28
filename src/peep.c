#include "peep.h"

struct stmt_pattern pattern_stmts[NUM_PSTMT] = {
#define PATTERN(a, fc, ...) __VA_ARGS__,
	PATTERN_TABLE
#undef PATTERN
};


struct pattern peephole_patterns[NUM_PATTERNS] = {
#define PATTERN(a, func, ...) {a, func},
	PATTERN_TABLE
#undef PATTERN
};

void optimize(struct ir_stmt *start)
{
	if (!start) return;
	struct pattern_state state;
	//state.callback = find_handle;
	state.splist = pattern_stmts;
	state.plist = peephole_patterns;
	state.num_spatts = NUM_PSTMT;
	state.num_patts = NUM_PATTERNS;
	while (start) {
		state.entry = start;
		start = find_stmt_match(&state);
		start = start->next;
	}
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
		if (compare_operands(ld->result, cur->result)) {
			ir_operand_free(cur->result);
			cur->result = copy(ld->arg1);
		}
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
