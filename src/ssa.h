#ifndef SSA_H
#define SSA_H

#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"

/*To find efficient places to put phi nodes,
 * the dominance frontier's need to be found*/
int intersect(struct bb **doms, int o1, int o2);
/*immediate dominator*/
struct bb *idom(struct bb *n);
/*Calculates dominance*/
struct bb **calc_dom(struct bb **bbs, int nbbs);
/*Calculates dominance frontiers*/
void calc_df(struct bb **bbs, struct bb **doms, int nbbs);
/*SSA conversion*/
void cfg_ssa(struct bb *bb);
void add_phi(struct bb *blk, struct symbol *s);
void add_phi_arg(struct bb *blk, struct symbol *s, int iter);
void set_phi(struct bb *bb);
void valid_sym(struct symbol *sym);
void set_ssa(struct bb **bbs, int nbbs);
void ssa_fmt(struct ir_stmt *st);

#endif
