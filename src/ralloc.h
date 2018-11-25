#ifndef RALLOC_H
#define RALLOC_H

#include "tac.h"
#include "cfg.h"
#include "color.h"

/*Register Allocation:
 * Eventual goal is graph coloring global register allocation,
 * but for now just local register allocation in basic blocks
 * */

int find_node(struct vertex **graph, int nvert, int tmp);
void make_edge(struct vertex *v1, struct vertex *v2);
struct vertex **add_node(struct vertex **graph, int *nvert, int tmp);
struct vertex **interference(struct bb *blk, int *num_nodes);

struct ir_operand **mem2reg(struct bb **bbs, int nbbs, int *num_map);
struct vertex **dedup_interference(struct vertex **graph, int *num_vert);
void proc_alloc(struct bb **bbs, int nbbs);
int temp_map(struct ir_operand **map, int nmap, struct symbol *var, int iter);
void repload(struct ir_stmt *cur, int temp, int ntemp);
void repstore(struct ir_stmt *cur, int temp, int ntemp);

#endif
