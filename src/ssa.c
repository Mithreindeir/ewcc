#include "ssa.h"

/*Dominators can be calculated in an array in postorder of the dominance
 * tree, which allows more efficient computation,
 * as well as implicit immediate dominator calculations, which help when
 * finding dominance frontiers.
 * Adapted from algorithm described by:
 * Keith D. Cooper, Timothy J. Harvey, and Ken Kennedy of Rice University
 * */
struct bb **calc_dom(struct bb **bbs, int nbbs)
{
	if (!nbbs || !bbs) return NULL;
	struct bb **doms = calloc(sizeof(struct bb*), nbbs);
	doms[0] = bbs[0];
	int nidom, same = 0;
	/*While the algorithm hasn't converged:*/
	while (!same) {
		same = 1;
		/*Loop through the cfg in reverse postorder*/
		for (int i = nbbs-1; i >= 1; i--) {
			struct bb *n = bbs[i];
			if (!n->pred) continue;
			nidom = n->pred[0]->order;
			for (int j = 1; j < n->npred; j++) {
				struct bb *p = n->pred[j];
				if (doms[p->order]) {
					nidom = intersect(doms, p->order, nidom);
				}
			}
			if (doms[n->order] != bbs[nidom]) {
				doms[n->order] =  bbs[nidom];
				same = 0;
			}
		}
	}
	return doms;
}

/*Walks up the dominator tree and finds the common relative between n1 and n2*/
int intersect(struct bb **doms, int o1, int o2)
{
	int s1=o1, s2=o2;
	while (o1 != o2) {
		while (o1 > o2 && doms[o1] && doms[o1]->order != o1) {
			o1 = doms[o1]->order;
			if (o1 == s1) break;
		}

		while (o2 > o1 && doms[o2] && doms[o2]->order != o2) {
			o2 = doms[o2]->order;
			if (s2 == o2) break;
		}
		if (s1==o1 && s2==o2) break;
		s1 = o1, s2 = o2;
	}
	return o1>o2?o2:o1;
}

/*Dominance frontier calculation based on the assumption
 * that a nodes N's predecessors must contain N in their dominance
 * frontier set unless they dominate N*/
void calc_df(struct bb **bbs, struct bb **doms, int nbbs)
{
	for (int i = 0; i < nbbs; i++) {
		struct bb *n = bbs[i];
		/*If there is only 1 predecessor, it must dominate it*/
		if (n->npred<=1) continue;
		for (int j = 0; j < n->npred; j++) {
			struct bb *p = n->pred[j], *r = p;
			/*Walk up the dominance tree by taking each nodes immediate dominator*/
			while (r  && r != doms[n->order]) {
				printf("df of %d += (%d)\n", r->ln, n->ln);
				r->frontier = bb_array_add(r->frontier, &r->nfront, n);
				/*add n to r's dominance frontier*/
				r = doms[r->order];
			}
		}
	}
}

void add_phi(struct bb *blk, struct symbol *s)
{
	for (int i = 0; i < blk->nphi; i++) {
		if (blk->phi_hdr[i].var == s) {
			return;
		}
	}
	blk->nphi++;
	if (!blk->phi_hdr) blk->phi_hdr = malloc(sizeof(struct phi));
	else blk->phi_hdr = realloc(blk->phi_hdr, sizeof(struct phi) * blk->nphi);
	struct phi p;
	p.var = s, p.piter = 0;
	p.iters = NULL, p.niter = 0;
	blk->phi_hdr[blk->nphi-1] = p;
}

void add_phi_arg(struct bb *blk, struct symbol *s, int iter)
{
	if (!iter) return;
	struct phi *p;
	for (int i = 0; i < blk->nphi; i++) {
		p = &blk->phi_hdr[i];
		if (blk->phi_hdr[i].var == s) {
			for (int j = 0; j < p->niter; j++)
				if (p->iters[j] == iter) return;
			p->niter++;
			if (!p->iters) p->iters = malloc(sizeof(int));
			else p->iters = realloc(p->iters, sizeof(int)*p->niter);
			p->iters[p->niter-1] = iter;
			return;
		}
	}
}
/*Calculate phi nodes by checking the variables a basic block uses,
 * then iterating over the dominance frontier of the block and adding phi nodes*/
void set_phi(struct bb *bb)
{
	struct ir_stmt *cur = bb->blk;
	int len = 0;
	while (cur && len < bb->len) {
		struct symbol *s = NULL;
		/*Every variable is only accessed through stores & loads*/
		if (cur->type == stmt_store || cur->type == stmt_load) {
			if (cur->arg1 && cur->arg1->type == oper_sym)
				s = cur->arg1->val.sym;
		}
		if (s) {
			for (int j = 0; j < bb->nfront; j++)
				add_phi(bb->frontier[j], s);
		}
		cur = cur->next, len++;
	}
}
/*Changes cfg block statements to ssa*/
void cfg_ssa(struct bb *bb)
{
	struct ir_stmt *cur = bb->blk;
	int len = 0;
	while (cur && len < bb->len) {
		/*Every variable is only accessed through stores & loads*/
		if (cur->type == stmt_store || cur->type == stmt_load) {
			ssa_fmt(cur);
		}
		cur = cur->next, len++;
	}
}

void set_ssa(struct bb **bbs, int nbbs)
{
	/*First add phi nodes*/
	for (int i = 0; i < nbbs; i++)
		set_phi(bbs[i]);
	/*Iteratively solve liveness info*/
	int change = 1;
	while (change) {
		change = 0;
		for (int i = nbbs-1; i >= 0; i--) {
			change = cfg_liveiter(bbs[i]) ?  1 : change;
		}
	}
	/*SSA Form*/
	for (int i = 0; i < nbbs; i++) {
		struct bb *b = bbs[i];
		for (int i = 0; i < b->nin; i++) {
			b->in[i]->iter = b->in[i]->val.sym->iter;
		}
		/*Set phi arguments basic on block inputs*/
		for (int k = 0; k < b->nin; k++) {
			if (b->in[k]->type != oper_sym) continue;
			struct symbol *s = b->in[k]->val.sym;
			add_phi_arg(b, s, b->in[k]->iter);
		}
		/*Increment the ssa iter for all the phi nodes*/
		for (int j = 0; j < b->nphi; j++) {
			struct phi *p = &b->phi_hdr[j];
			p->var->iter++;
			p->piter = p->var->iter;
			for (int i = 0; i < b->nin; i++) {
				if (p->var == b->in[i]->val.sym)
					b->in[i]->iter = p->piter;
			}
		}
		cfg_ssa(bbs[i]);
		for (int i = 0; i < b->nout; i++) {
			b->out[i]->iter = b->out[i]->val.sym->iter;
		}
		/*Set DF phi arguments based on block outputs*/
		for (int j = 0; j < b->nfront; j++) {
			struct bb *f = b->frontier[j];
			if (f == b) continue;
			for (int k = 0; k < b->nout; k++) {
				if (b->out[k]->type != oper_sym) continue;
				struct symbol *s = b->out[k]->val.sym;
				add_phi_arg(f, s, b->out[k]->iter);
			}
		}
	}
}

/*All invalid symbols are automatically spilled.
 * Invalid symbols are arrays, symbols whose address is taken
 * and symbols with sizes bigger than registers sizes*/
void valid_sym(struct symbol *sym)
{
	if (!sym) return;
	int valid = 1;
	if (sym->size > REG_SIZE) valid = 0;
	if (sym->offset < 0) valid = 0;
	if (!sym->allocd) valid = 0;
	sym->spilled = !valid;
}

/*For every variable store, increment its ssa iter,
 * and for every read set the ssa iter to the ir operand*/
void ssa_fmt(struct ir_stmt *st)
{
	/*no pointer ssa, only directly used variables*/
	if (!st || !st->arg1 || st->arg1->type != oper_sym) return;
	int write = st->type == stmt_store;
	struct symbol *sym = st->arg1->val.sym;
	valid_sym(sym);
	if (sym->spilled) return;
	if (sym->offset < 0) return;
	if (write)
		sym->iter++;
	st->arg1->iter = sym->iter;
}
