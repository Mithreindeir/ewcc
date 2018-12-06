#ifndef RALLOC_H
#define RALLOC_H

#include <assert.h>
#include "tac.h"
#include "cfg.h"
#include "color.h"

#define OVERLAP(a, b) (a->def < b->kill && a->kill > b->def)

/*Register Allocation:
 * Eventual goal is graph coloring global register allocation,
 * but for now just local register allocation in basic blocks
 * */

int find_node(struct vertex **graph, int nvert, int tmp);
struct vertex **add_node(struct vertex **graph, int *nvert, int tmp);

struct ir_stmt *reduce_mem(struct ir_stmt *cur, int temp);
struct ir_operand **mem2reg(struct bb **bbs, int nbbs, int *num_map);
struct vertex **interference(struct bb *blk, struct vertex ** graph, int *num_nodes);
int temp_map(struct ir_operand **map, int nmap, struct symbol *var, int iter);

void repload(struct ir_stmt *cur, int temp, int ntemp);
void repstore(struct ir_stmt *cur, int temp, int ntemp);
void insloadstore(struct ir_stmt *cur, int stemp, int temp, struct ir_operand *rep);
void repregs(struct ir_stmt *cur, struct vertex **vert, int nvert);
void proc_alloc(struct bb **bbs, int nbbs);

#endif
