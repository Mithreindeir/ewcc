#include "live.h"

struct bb *bb_init(struct ir_stmt *s, struct ir_stmt **next)
{
	struct bb *blk = malloc(sizeof(struct bb));

	blk->succ = NULL, blk->pred = NULL;
	blk->nsucc = 0, blk->npred = 0;

	blk->use = NULL, blk->def = NULL;
	blk->nuse = 0, blk->ndef = 0;

	blk->graph = NULL;
	blk->nvert = 0;

	blk->in = NULL, blk->out = NULL;
	blk->nin = 0, blk->nout = 0;
	blk->visited = 0;

	blk->blk = s;
	int len = 1;
	/*Basic blocks can start at labels but must end at a jump or any label not at the start*/
	while (s) {
		if (s->type == stmt_cgoto || s->type == stmt_ugoto) {
			break;
		}
		if (s->type == stmt_label && len != 1) {
			/*Do not include this*/
			s = s->prev;
			len--;
			break;
		}
		len++;
		s = s->next;
	}
	*next = s ? s->next : NULL;
	blk->lst = s;
	blk->len = len;

	bb_set(blk);

	return blk;
}

void bb_free(struct bb *blk)
{
	if (!blk) return;
	for (int i = 0; i < blk->nvert; i++)
		vertex_free(blk->graph[i]);
	free(blk->graph);
	free(blk->succ);
	free(blk->pred);
	free(blk->use);
	free(blk->def);
	free(blk);
}

int bb_get_treg(struct bb *blk, int reg)
{
	for (int i = 0; i < blk->nvert; i++) {
		if (blk->graph[i]->ocolor == reg) return i;
	}
	blk->nvert++;
	if (!blk->graph) blk->graph = malloc(sizeof(struct vertex*));
	else blk->graph = realloc(blk->graph, sizeof(struct vertex*)*blk->nvert);
	struct vertex *v = vertex_init();
	v->ocolor = reg;
	blk->graph[blk->nvert-1] = v;
	return blk->nvert-1;
}

void bb_addpred(struct bb *blk, struct bb *pred)
{
	blk->npred++;
	if (!blk->pred) blk->pred = malloc(sizeof(struct bb*));
	else blk->pred = realloc(blk->pred, blk->npred * sizeof(struct bb*));
	blk->pred[blk->npred-1] = pred;
}

void bb_addsucc(struct bb *blk, struct bb *succ)
{
	blk->nsucc++;
	if (!blk->succ) blk->succ = malloc(sizeof(struct bb*));
	else blk->succ = realloc(blk->succ, blk->nsucc * sizeof(struct bb*));
	blk->succ[blk->nsucc-1] = succ;
}

void def(struct bb *blk, struct ir_stmt *s)
{
	if (s->result && s->result->type == oper_reg) {
		/*Update def list*/
		intarr_add(&blk->def, &blk->ndef, s->result->val.virt_reg);
	}
}

void use(struct bb *blk, struct ir_stmt *s)
{
	if (s->arg1 && s->arg1->type == oper_reg)
		intarr_add(&blk->use, &blk->nuse, s->arg1->val.virt_reg);
	if (s->arg2 && s->arg2->type == oper_reg)
		intarr_add(&blk->use, &blk->nuse, s->arg2->val.virt_reg);
}

void bb_set(struct bb *blk)
{
	struct ir_stmt *cur = blk->blk;

	int iter = 0;
	while (cur && iter < blk->len) {
		def(blk, cur);
		use(blk, cur);
		iter++;
		cur = cur->next;
	}

	for (int i = 0; i < blk->ndef; i++)
		(void)bb_get_treg(blk, blk->def[i]);
	for (int i = 0; i < blk->nuse; i++)
		(void)bb_get_treg(blk, blk->use[i]);

	cur = blk->blk;
	iter = 0;
	int *defd = calloc(blk->nvert, sizeof(int)), ndefd = blk->nvert;
	int *kill = calloc(blk->nvert, sizeof(int)), nkill = blk->nvert;

	while (cur && iter < blk->len) {
		if (cur->result && cur->result->type == oper_reg) {
			defd[bb_get_treg(blk, cur->result->val.virt_reg)] = iter+1;
		}
		/*set kill*/
		if (cur->arg1 && cur->arg1->type == oper_reg) {
			kill[bb_get_treg(blk, cur->arg1->val.virt_reg)] = iter;
		}
		if (cur->arg2 && cur->arg2->type == oper_reg) {
			kill[bb_get_treg(blk, cur->arg2->val.virt_reg)] = iter;
		}
		cur = cur->next;
		iter++;
	}

	/*Construct interference graph*/
	for (int i = 0; i < ndefd; i++) {
		for (int j = 0; j < nkill; j++) {
			if (j == i) continue;
			if (kill[j] < defd[i] || defd[j] > kill[i])
				continue;
			struct vertex *v2 = blk->graph[j];
			struct vertex *v = blk->graph[i];
			int added = 0;
			for (int k = 0; k < v->num_siblings; k++) {
				if (v->siblings[k] == v2) {
					added = 1;
					break;
				}
			}
			if (!added) {
				add_edge(v, v2);
			}
		}
	}

	free(kill);
	free(defd);
}

struct bb **cfg(struct ir_stmt *entry, int *len)
{
	int *labels = NULL;
	struct bb **bbs = NULL;
	int nbb = 0;

	struct ir_stmt *cur = entry;
	struct ir_stmt *next = NULL;
	/*Construct basic blocks from linear IR list*/
	while (cur) {
		int label = -1;
		struct bb *cbb = bb_init(cur, &next);
		if (nbb && cur->prev) {
			if (cur->prev->type != stmt_ugoto) {
				bb_addpred(cbb, bbs[nbb-1]);
				bb_addsucc(bbs[nbb-1], cbb);
			}
		}
		if (cur->type == stmt_label) label = cur->label;

		nbb++;
		if (nbb==1) {
			bbs = malloc(sizeof(struct bb*));
			labels = malloc(sizeof(int));
		} else {
			bbs = realloc(bbs, sizeof(struct bb*)*nbb);
			labels = realloc(labels, sizeof(int)*nbb);
		}
		bbs[nbb-1] = cbb;
		labels[nbb-1] = label;

		cur = next;
	}
	/*Add edges from labels/jumps to make a control flow graph*/
	for (int i = 0; i < nbb; i++) {
		struct ir_stmt *last = bbs[i]->lst;
		if (!last) continue;
		int jump = -1;
		if (last->type == stmt_ugoto || last->type == stmt_cgoto)
			jump = last->label;

		struct bb *edge = NULL;
		for (int j = 0; j < nbb; j++) {
			edge = labels[j]==jump ? bbs[j] : NULL;
			if (edge) break;
		}
		if (jump != -1 && edge) {
			bb_addsucc(bbs[i], edge);
			bb_addpred(edge, bbs[i]);
		}
	}

	/*Get rid of dangling blocks (simple dead code analysis could happen here)*/
	for (int i = 0; i < nbb; i++) {
		if (!bbs[i]->pred && !bbs[i]->succ) {
			bb_free(bbs[i]);
			bbs[i] = NULL;
		}
	}

	free(labels);

	*len = nbb;
	return bbs;
}

void intarr_add(int **arr, int *num, int val)
{
	int n = *num, *array = *arr;
	n++;
	if (n == 1) {
		array = malloc(sizeof(int));
	} else {
		array = realloc(array, sizeof(int) * n);
	}

	array[n - 1] = val;

	*num = n;
	*arr = array;
}
