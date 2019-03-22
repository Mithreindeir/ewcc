#include "cfg.h"

struct bb *bb_init(struct ir_stmt *s, struct ir_stmt **next)
{
	if (!s) return NULL;
	struct bb *blk = malloc(sizeof(struct bb));

	blk->frontier = NULL;
	blk->nfront = 0;
	blk->succ = NULL, blk->pred = NULL;
	blk->nsucc = 0, blk->npred = 0;
	blk->phi_hdr = NULL, blk->nphi = 0;
	blk->in = NULL, blk->nin = 0;
	blk->out = NULL, blk->nout = 0;

	blk->ln = 0;

	blk->visited = 0;
	blk->order = 0;
	blk->blk = s;
	int len = 1;
	/*Basic blocks can start at labels but must end at a jump or any label not at the start */
	while (s) {
		if (s->type == stmt_ret || s->type == stmt_cgoto || s->type == stmt_ugoto) {
			break;
		}
		if ((s->type == stmt_func || s->type == stmt_label) && len != 1) {
			s = s->prev;
			//len--;
			break;
		}
		len++;
		s = s->next;
	}
	*next = s ? s->next : NULL;
	blk->lst = s;
	blk->len = len;

	/*Get the line number */
	struct ir_stmt *cnt = blk->blk;
	int pl = 0;
	while ((++pl, (cnt=cnt->prev)));
	blk->ln = pl;

	return blk;
}

void bb_free(struct bb *blk)
{
	if (!blk)
		return;
	for (int i = 0; i < blk->nphi; i++) {
		free(blk->phi_hdr[i].iters);
	}
	for (int i = 0; i < blk->nout; i++)
		ir_operand_free(blk->out[i]);
	for (int i = 0; i < blk->nin; i++)
		ir_operand_free(blk->in[i]);
	free(blk->out);
	free(blk->in);
	free(blk->phi_hdr);
	free(blk->frontier);
	free(blk->succ);
	free(blk->pred);
	free(blk);
}

struct bb **bb_array_add(struct bb **bbs, int *nbbs, struct bb *elem)
{
	int num = *nbbs + 1;
	if (num==1) bbs = malloc(sizeof(struct bb*));
	else bbs = realloc(bbs, sizeof(struct bb*)*num);
	*nbbs = num;
	bbs[num-1] = elem;
	return bbs;
}

struct bb **bb_array_del(struct bb **bbs, int *nbbs, int idx)
{
	int num = *nbbs;
	if ((idx+1) < num) {
		memmove(bbs+idx, bbs+idx+1, (num-idx-1)*sizeof(*bbs));
	}
	num--;
	if (!num) free(bbs);
	else bbs = realloc(bbs, sizeof(*bbs)*num);
	*nbbs = num;
	return bbs;
}

int in_oper_array(struct ir_operand **arr, int narr, struct ir_operand *op)
{
	for (int i = 0; i < narr; i++) {
		if (arr[i]->val.sym == op->val.sym) return 1;
		/*if (arr[i]->val.sym == op->val.sym && arr[i]->iter <= op->iter) return 1;
		if (arr[i]->val.sym == op->val.sym && op->iter > arr[i]->iter) {
			arr[i]->iter = op->iter;
			return 1;
		}*/
	}
	return 0;
}

/*Iteratively add to inputs/outputs*/
int cfg_liveiter(struct bb *bb)
{
	int change = 0;
	/*out[b] = U in(succ(b))*/
	/*in[b] = use[b] U (out[b] - def[b])*/
	struct ir_operand **in=NULL, **out=NULL;
	struct ir_operand **use=NULL, **def=NULL;
	int nin=0, nout=0, nuse=0, ndef=0;
	for (int i = 0; i < bb->nsucc; i++) {
		struct bb *s = bb->succ[i];
		for (int j = 0; j < s->nin; j++) {
			if (in_oper_array(out, nout, s->in[j])) continue;
			nout++;
			if (!out) out = malloc(sizeof(*out));
			else out = realloc(out, sizeof(*out)*nout);
			out[nout-1] = copy(s->in[j]);
		}
	}
	struct ir_stmt *cur = bb->blk;
	int len = 0;
	while (cur && len < bb->len) {
		/*Every variable is only accessed through stores & loads*/
		if ((cur->type == stmt_store || cur->type == stmt_load) && cur->arg1->type == oper_sym) {
			int used = in_oper_array(use, nuse, cur->arg1);
			int defined = in_oper_array(def, ndef, cur->arg1);
			if (cur->type == stmt_load && !used) {
				nuse++;
				if (!use) use = malloc(sizeof(*use));
				else use = realloc(use, sizeof(*use)*nuse);
				use[nuse-1] = copy(cur->arg1);
			}
			if (cur->type == stmt_store && !defined) {
				ndef++;
				if (!def) def = malloc(sizeof(*def));
				else def = realloc(def, sizeof(*def)*ndef);
				def[ndef-1] = copy(cur->arg1);
			}
		}
		cur = cur->next, len++;
	}
	/*in[b] = use[b] U (out[b] - def[b])*/
	if (nuse > 0) {
		nin = nuse;
		in = malloc(sizeof(*in) * nin);
	}
	for (int i = 0; i < nuse; i++)
		in[i] = copy(use[i]);
	for (int i = 0; i < nout; i++) {
		if (in_oper_array(def, ndef, out[i]) || in_oper_array(in, nin, out[i])) continue;
		nin++;
		if (nin == 1) in = malloc(sizeof(*in)*nin);
		else in = realloc(in, sizeof(*in) * nin);
		in[nin-1] = copy(out[i]);
	}
	if (nin != bb->nin || nout != bb->nout) {
		change = 1;
	} else {
		for (int i = 0; i < bb->nin; i++) {
			if (!in_oper_array(in, nin, bb->in[i])) {
				change = 1;
				break;
			}
		}
		for (int i = 0; i < bb->nout; i++) {
			if (!in_oper_array(out, nout, bb->out[i])) {
				change = 1;
				break;
			}
		}
	}
	for (int i = 0; i < bb->nin; i++)
		ir_operand_free(bb->in[i]);
	for (int i = 0; i < bb->nout; i++)
		ir_operand_free(bb->out[i]);
	free(bb->out);
	free(bb->in);
	bb->in = in, bb->out = out;
	bb->nin = nin, bb->nout = nout;
	for (int i = 0; i < nuse; i++)
		ir_operand_free(use[i]);
	for (int i = 0; i < ndef; i++)
		ir_operand_free(def[i]);
	free(use);
	free(def);
	return change;
}

struct bb **cfg_dead(struct bb **bbs, int *nbbs)
{
	int nbb = *nbbs;
	/*Get rid of dangling blocks, ignoring entry point */
	struct ir_stmt *cur, *next, *prev;
	for (int i = 1; i < nbb; i++) {
		if (!bbs[i])
			continue;
		if (!bbs[i]->pred&&!bbs[i]->succ) {
			/*Unlink dead code */
			prev=(next=(cur=bbs[i]->blk))->prev;
			if (prev) {
				int ln = 0;
				while (cur && ln < bbs[i]->len) {
					next = cur->next;
					ir_stmt_free(cur);
					ln++;
					cur = next;
				}
				prev->next = cur;
				if (cur) {
					cur->prev = prev;
				}
			}
			bb_free(bbs[i]);
			bbs[i] = NULL;
			bbs = bb_array_del(bbs, &nbb, i);
			i--;
		}
	}
	*nbbs = nbb;
	return bbs;
}

/*Add edges from labels/jumps to add edges to control flow graph*/
void cfg_edge(struct bb **bbs, int nbb, int *labels)
{
	for (int i = 0; i < nbb; i++) {
		if (!bbs[i])
			continue;
		struct ir_stmt *last = bbs[i]->lst;
		if (!last)
			continue;
		int jump = -1;
		if (last->type == stmt_ugoto || last->type == stmt_cgoto)
			jump = last->label;

		struct bb *edge = NULL;
		for (int j = 0; j < nbb; j++) {
			edge = labels[j] == jump ? bbs[j] : NULL;
			if (edge)
				break;
		}
		if (jump != -1 && edge) {
			ADD_SUCC(bbs[i], edge);
			ADD_PRED(edge, bbs[i]);
		}
	}

}

/*Sort the cfg array in postorder*/
void cfg_postorder(struct bb **bbs, int nbb)
{
	for (int i = 0; i < nbb; i++) bbs[i]->visited = 0;
	int cnt = nbb-1;
	/*Set the ordering tags on the nodes*/
	postorder(bbs[0], &cnt);
	/*The postorder tags are also a direct mapping to array indices*/
	struct bb *tmp;
	int o = 0;
	for (int i = 0; i < nbb; i++) {
		o = bbs[i]->order;
		if (i != o) {
			tmp = bbs[o];
			bbs[o] = bbs[i];
			bbs[i] = tmp;
			i--;
		}
	}
}

void postorder(struct bb *top, int *cnt)
{
	if (!top || top->visited) return;
	top->visited = 1;
	for (int i = 0; i < top->nsucc; i++)
		postorder(top->succ[i], cnt);
	top->order = *cnt;
	(*cnt)--;
}

struct bb **cfg(struct ir_stmt *e, int *len)
{
	static struct ir_stmt *entry = NULL;
	if (e) entry = e;
	if (!entry) return NULL;
	int *labels = NULL;
	struct bb **bbs = NULL;
	int nbb = 0;

	struct ir_stmt *cur = entry;
	struct ir_stmt *next = NULL;
	/*Construct basic blocks from linear IR list */
	while (cur) {
		int label = -1;
		/*Each function gets it's own cfg*/
		if (cur != entry && cur->type == stmt_func) {
			break;
		}
		struct bb *cbb = bb_init(cur, &next);
		if (nbb && cur->prev) {
			if (cur->prev->type != stmt_ugoto) {
				ADD_PRED(cbb, bbs[nbb-1]);
				ADD_SUCC(bbs[nbb-1], cbb);
			}
		}
		if (cur->type == stmt_label)
			label = cur->label;

		bbs = bb_array_add(bbs, &nbb, cbb);

		if (nbb==1) labels = malloc(sizeof(int));
		else labels = realloc(labels, sizeof(int) * nbb);
		labels[nbb - 1] = label;

		if (cur->type == stmt_ret) break;
		cur = next;
	}
	entry = cur;
	if (!bbs) return NULL;
	cfg_edge(bbs, nbb, labels);
	bbs = cfg_dead(bbs, &nbb);
	cfg_postorder(bbs, nbb);

	free(labels);
	*len = nbb;
	return bbs;
}
