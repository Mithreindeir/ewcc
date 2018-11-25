#include "match.h"

struct ir_stmt *find_stmt_match(struct pattern_state *state)
{
	if (!state->entry)
		return NULL;
	int offset = 0, ns = 0;
	struct ir_stmt *iter = NULL, *a = state->entry;

	for (int i = 0; i < state->num_patts; i++) {
		ns = state->plist[i].num_stmt;
		iter = a;
		if (offset + ns > state->num_spatts) {
			break;
		}
		int nm;
		/*first check if the statement types are right */
		for (nm = 0; nm < ns && iter; nm++, iter = iter->next) {
			if (!match_pattern
			    (state->splist + nm + offset, ns - nm, iter)) {
				break;
			}
		}
		if (nm == ns) {
			return state->callback(a, ns,
					       state->plist[i].value);
		}

		offset += ns;
	}
	return NULL;
}


int match_pattern(struct stmt_pattern *spat, int max, struct ir_stmt *st)
{
	if (st->type != (*spat).st_type)
		return 0;

	struct ir_stmt *src = NULL, *dst = NULL;
	struct ir_stmt *cur = st;
	int it = 0;
	while (cur && it < max) {
		//if (it > 1 && !src)
		if (src && dst)
			break;
		if (!src&&(spat[it].flags & SRC))
			src = cur;
		if (!dst&&(spat[it].flags & DST))
			dst = cur;
		it++;
		cur = cur->next;
	}
	if (!src || !dst)
		src = st, dst = st;
	struct stmt_pattern pat = *spat;

	if ((pat.flags & LBL))
		return dst->label == src->label;

	if (!match_operand(dst, src->result, pat.result))
		return 0;
	if (!match_operand(dst, src->arg1, pat.arg1))
		return 0;
	if (!match_operand(dst, src->arg2, pat.arg2))
		return 0;

	return 1;
}

int match_operand(struct ir_stmt *st, struct ir_operand *a,
		  enum oper_pattern pat)
{
	if (!a)
		return 1;
	if (pat == opat_ignore)
		return 1;
	if (pat == opat_imm && a->type == oper_cnum)
		return 1;
	if (pat == opat_reg && a->type == oper_reg)
		return 1;
	if (pat == opat_mem && a->type == oper_sym)
		return 1;

	if (pat == opat_result)
		return compare_operands(a, st->result);
	if (pat == opat_arg1)
		return compare_operands(a, st->arg1);
	if (pat == opat_arg2)
		return compare_operands(a, st->arg2);

	return 0;
}

int compare_operands(struct ir_operand *a, struct ir_operand *b)
{
	if (!b || !a)
		return 0;
	if (a->type != b->type)
		return 0;
	enum oper_type t = a->type;
	if (t == oper_reg && a->val.virt_reg == b->val.virt_reg)
		return 1;
	if (t == oper_cnum && a->val.constant == b->val.constant)
		return 1;
	if (t == oper_sym && a->val.sym && b->val.sym
	    && !strcmp(a->val.sym->identifier, b->val.sym->identifier))
		return 1;
	return 0;
}
