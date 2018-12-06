#ifndef CFG_H
#define CFG_H

#include <stdio.h>
#include <stdlib.h>
#include "tac.h"

#define ADD_PRED(n, p) (n)->pred = bb_array_add((n)->pred, &(n)->npred, (p));
#define ADD_SUCC(n, s) (n)->succ = bb_array_add((n)->succ, &(n)->nsucc, (s));

struct phi {
	int piter;//the iter that this phi node holds
	struct symbol *var;
	int *iters, niter;
};

/*Basic Block*/
struct bb {
	struct ir_stmt *blk, *lst;
	int len;

	/*Out info, used to calculate phi arguments*/
	struct ir_operand **in, **out;
	int nin, nout;

	/*Predecessors, successors, and dominance frontiers*/
	struct bb **pred, **succ, **frontier;
	int nsucc, npred, nfront;

	int visited, ln, order;
	/*Every basic block can have header of phi instruction*/
	struct phi *phi_hdr;
	int nphi;
};

struct bb *bb_init(struct ir_stmt *s, struct ir_stmt **next);
void bb_free(struct bb *blk);

struct bb **bb_array_add(struct bb **bbs, int *nbbs, struct bb *elem);
struct bb **bb_array_del(struct bb **bbs, int *nbbs, int idx);

struct bb **cfg_dead(struct bb **bbs, int *nbbs);
struct bb **cfg(struct ir_stmt *entry, int *len);
int cfg_liveiter(struct bb *bb);
void cfg_edge(struct bb **bbs, int nbb, int *labels);
void cfg_postorder(struct bb **bbs, int nbb);
void postorder(struct bb *top, int *cnt);

#endif
