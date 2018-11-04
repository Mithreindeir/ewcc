#ifndef LIVE_H
#define LIVE_H

#include <stdio.h>
#include <stdlib.h>
#include "tac.h"
#include "ralloc.h"

/*Liveness analysis on IR
 * Algorithm:
 * Live-In: if temp t is in set of use in node or path from n to node but doesnt define it
 * t E in[N]
 * Live-Out: if tem pt is in set of use of successors of n
 * t E out[N]
 *
 *
 * in[block] = out[pred0]  ... intersect out[predn]
 * out[block] = (in[block] - def[block]) | def[block]
 *
 * A blocks input's are the used variables that arent defined, or the sum of the predecessors
 * inputs
 * A blocks outputs are all the defined plus the inputs minus the redefined inputs
 *
 * */

/*Basic Block*/
struct bb {
	struct ir_stmt *blk, *lst;
	int len;
	/*Variables that are attempted to get stored in registers */
	struct ir_operand **rmap;
	int num_op;

	/*Successors and Predeccessors of the basic block */
	struct bb **succ, **pred;
	int nsucc, npred;

	/*The stmt # where a temp is last used */
	int *temp, ntemp;
	/*Use/def list */
	int *use, nuse, *def, ndef;
	/*For now local register allocation, so just keep interference graph on basic blocks */
	struct vertex **graph;
	int nvert;

	/*IR Register input/outputs */
	int *in, nin, *out, nout;
	int visited;
};

struct bb *bb_init(struct ir_stmt *s, struct ir_stmt **next);
void bb_free(struct bb *blk);

void bb_addpred(struct bb *blk, struct bb *pred);
void bb_addsucc(struct bb *blk, struct bb *succ);

void def(struct bb *blk, struct ir_stmt *s);
void use(struct bb *blk, struct ir_stmt *s);
/*Calculate use/def*/
void bb_set(struct bb *blk);
/*Overwrite temp registers*/
void bb_creg(struct bb *blk);
/*Adds a value to an int array*/
void intarr_add(int **arr, int *num, int val);

struct bb **cfg(struct ir_stmt *entry, int *len);
int bb_get_treg(struct bb *blk, int reg);

#endif
