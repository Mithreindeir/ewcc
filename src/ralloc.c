#include "ralloc.h"

int find_node(struct vertex **graph, int nvert, int tmp)
{
	int idx = -1, i = 0;
	for (i = 0; i < nvert; i++) {
		if (graph[i]->ocolor == tmp) {
			idx = i;
			break;
		}
	}
	return idx;
}

struct vertex **add_node(struct vertex **graph, int *nvert, int tmp)
{
	int num = *nvert+1;
	if (!graph) graph = malloc(sizeof(struct vertex*));
	else graph = realloc(graph, sizeof(struct vertex*)*num);
	*nvert = num;
	struct vertex *v = vertex_init();
	v->ocolor = tmp;
	graph[num-1] = v;
	return graph;
}

void repload(struct ir_stmt *cur, int temp, int ntemp)
{
	cur = cur->next;
	while (cur) {
		if (cur->type == stmt_label || cur->type == stmt_ugoto || cur->type == stmt_cgoto) {
		       	break;
		}

		if (cur->arg1 && cur->arg1->type == oper_reg) {
			if (cur->arg1->val.virt_reg == temp) {
				cur->arg1->val.virt_reg = ntemp;
			}
		}
		if (cur->arg2 && cur->arg2->type == oper_reg) {
			if (cur->arg2->val.virt_reg == temp) {
				cur->arg2->val.virt_reg = ntemp;
			}
		}
		if (cur->result && cur->result->type == oper_reg) {
			if (cur->result->val.virt_reg == temp) break;
			if (cur->result->val.virt_reg == ntemp) break;
		}

		cur = cur->next;
	}
}

void repstore(struct ir_stmt *cur, int temp, int ntemp)
{
	cur = cur->prev;
	while (cur) {
		if (cur->type == stmt_label || cur->type == stmt_ugoto || cur->type == stmt_cgoto) {
		       	break;
		}
		if (cur->arg1 && cur->arg1->type == oper_reg) {
			if (cur->arg1->val.virt_reg == temp) {
				cur->arg1->val.virt_reg = ntemp;
			}
		}
		if (cur->arg2 && cur->arg2->type == oper_reg) {
			if (cur->arg2->val.virt_reg == temp) {
				cur->arg2->val.virt_reg = ntemp;
			}
		}
		if (cur->result && cur->result->type == oper_reg) {
			if (cur->result->val.virt_reg == temp) {
				cur->result->val.virt_reg = ntemp;
				break;
			}
			if (cur->result->val.virt_reg == ntemp) break;
		}

		cur = cur->prev;
	}
}

/*Basic Block local interference*/
struct vertex **interference(struct bb *blk, struct vertex ** graph, int *num_nodes)
{
	int nvert = *num_nodes;

	struct ir_stmt *cur = blk->blk;
	int len = 0;
	/*Create graph vertices, and liveness info*/
	while (cur && len < blk->len) {
		struct ir_operand *op[3] = { cur->result, cur->arg1, cur->arg2 };
		for (int i = 0; i < 3; i++) {
			if (!op[i] || op[i]->type != oper_reg)
				continue;
			int tmp = op[i]->val.virt_reg;
			int idx = find_node(graph, nvert, tmp);
			if (!i && idx == -1) {
				graph = add_node(graph, &nvert, tmp);
				graph[nvert-1]->def = len+blk->ln;
				graph[nvert-1]->kill = len+blk->ln;
			} else if (i && (idx=find_node(graph, nvert, tmp)) != -1) {
				graph[idx]->kill = len+blk->ln;
			}
		}
		cur = cur->next, len++;
	}
	*num_nodes = nvert;
	return graph;
}

int temp_map(struct ir_operand **map, int nmap, struct symbol *var, int iter)
{
	for (int i = 0; i < nmap; i++) {
		if (map[i]->iter == iter) {
			if (map[i]->type == oper_sym) {
				if (map[i]->val.sym == var)
					return -i-1;
			}
		}
	}
	return 0;
}

/*Removes unnecessary loads and stores when the memory operand is being
 * pushed into a register*/
struct ir_stmt *reduce_mem(struct bb **bbs, int nbbs, int idx, struct ir_stmt *cur, int temp)
{
	struct bb *b = bbs[idx];
	if (cur->type == stmt_load) {
		/*Replace the loaded register with the ssa one*/
		repload(cur, cur->result->val.virt_reg, temp);
		struct ir_stmt *n = cur->prev;
		unlink(cur);
		b->len--;
		for (int i = idx+1; i < nbbs; i++)
			bbs[i]->ln--;
		ir_stmt_free(cur);
		cur = n;
	} else if (cur->type == stmt_store) {
		/*If the store is not with temps, don't replace*/
		if (cur->arg2 && cur->arg2->type == oper_reg && cur->arg2->val.virt_reg >= 0) {
			int r = cur->arg2->val.virt_reg;
			repstore(cur, r, temp);
			struct ir_stmt *n = cur->prev;
			unlink(cur);
			b->len--;
			for (int i = idx+1; i < nbbs; i++)
				bbs[i]->ln--;
			ir_stmt_free(cur);
			cur = n;
		} else {
			cur->result = from_reg(temp);
			ir_operand_free(cur->arg1);
			cur->arg1 = cur->arg2, cur->arg2 = NULL;
			cur->type = stmt_move;
		}
	}
	return cur;
}

/*Puts a memory variable into a temporary register if it is viable to be spilled*/
/*Memory to register. Pushes memory variables into registers, and writes the mapping
 * into an array*/
struct ir_operand **mem2reg(struct bb **bbs, int nbbs, int *num_map) {
	/*All previously defined temporary registers use positive values,
	 * so use negative numbers as a distinction for memory variables */
	int ssa = -1;
	struct ir_operand **map = NULL, *mem = NULL;
	int nmap = 0;
	for (int i = 0; i < nbbs; i++) {
		struct bb *b = bbs[i];
		struct ir_stmt *cur = b->blk;
		int len = 0;
		int blen = b->len;
		/*Iterate through load/store statments and put memory into registers*/
		while (cur && len < blen) {
			if (cur->type == stmt_store || cur->type == stmt_load) {
				if (cur->arg1 && cur->arg1->type == oper_sym)
					mem = cur->arg1;
			}
			if (mem && !mem->val.sym->spilled) {
				/*Assign reg if not already in one*/
				int temp = temp_map(map, nmap, mem->val.sym, mem->iter);
				/*Add operand to map*/
				if (!temp) {
					nmap++;
					if (!map) map = malloc(sizeof(*map));
					else map = realloc(map, sizeof(*map) * nmap);
					map[nmap-1] = copy(mem);
					temp = ssa--;
				}
				cur = reduce_mem(bbs, nbbs, i, cur, temp);
			}
			cur = cur->next, len++, mem=NULL;
		}
	}
	*num_map = nmap;
	return map;
}

/*Procedural Wide Register allocation.
 * Takes CFG in SSA form as input*/
void proc_alloc(struct bb **bbs, int nbbs)
{
	int nmap = 0;
	struct ir_operand **map = mem2reg(bbs, nbbs, &nmap);
			for (int i = 0; i < nbbs; i++) {
				bb_debug(bbs[i]);
			}

	/*Then Basic Block local register graphs are made*/
	struct vertex **vert = NULL;
	int nvert = 0;
	int nidx, oidx;
	for (int i = 0; i < nbbs; i++) {
		int lvert = nvert;
		vert = interference(bbs[i], vert, &nvert);
		/*Outputs get all of the nodes as edge*/
		for (int j = 0; j < bbs[i]->nout; j++) {
			struct ir_operand *o = bbs[i]->out[j];
			if (o->type != oper_sym) continue;
			if (!(oidx=temp_map(map, nmap, o->val.sym, o->iter)))
				continue;
			if ((nidx=find_node(vert, nvert, oidx)) < 0)
				nidx = nvert, vert = add_node(vert, &nvert, oidx);
			struct vertex *ov = vert[nidx];
			for (int l = lvert; l < nvert; l++) {
				if (vert[l] == ov) continue;
				if (ov->def > vert[l]->kill) continue;
				add_edge(ov, vert[l]);
			}
		}
	}
	/*All overlapping live ranges get edge*/
	for (int i = 0; i < nvert; i++) {
		for (int j = 0; j < nvert; j++) {
			if (j == i) continue;
			if (OVERLAP(vert[i], vert[j])) add_edge(vert[i], vert[j]);
		}
	}

	/*Phi node arguments get coalesced*/
	for (int i = 0; i < nbbs; i++) {
		for (int j = 0; j < bbs[i]->nphi; j++) {
			struct phi *p = &bbs[i]->phi_hdr[j];
			if (!(oidx=temp_map(map, nmap, p->var, p->piter)))
				continue;
			if ((nidx=find_node(vert, nvert, oidx)) < 0)
				nidx = nvert, vert = add_node(vert, &nvert, oidx);
			struct vertex *rv = vert[nidx];
			for (int k = 0; k < p->niter; k++) {
				if (!(oidx=temp_map(map, nmap, p->var, p->iters[k])))
					continue;
				if ((nidx=find_node(vert, nvert, oidx)) < 0)
					nidx = nvert, vert = add_node(vert, &nvert, oidx);
				struct vertex *av = vert[nidx];
				coalesce(rv, av);
			}
		}
	}

	int clrs = 6;
	color_graph(vert, nvert, clrs);
	int spill = 0;
	/*Check for spilled registers*/
	for (int i = 0; i < nvert; i++) {
		if (vert[i]->color == clrs && vert[i]->ocolor < 0) {
			/*Spill back to SSA reg*/
			vert[i]->color = vert[i]->ocolor;
			if (vert[i]->color > 0) {
				printf("Spilling temps isn't allowed yet\n");
				exit(1);
			}
			spill = 1;
		}
	}

	if (spill) {
		int max=0;
		for (int i = 0; i < nvert; i++) {
			if (vert[i]->ocolor > max) max = vert[i]->ocolor;
		}
		/*Remove spilled value from interference graph
		 * Create a new scratch reg and replace it in the interference graph
		 * but no coalescing*/
		/*Retry to color graph and repeat if needed*/
		for (int i = 0; i < nvert; i++) {
			if (vert[i]->ocolor == vert[i]->color && vert[i]->ocolor < 0) {
				int st = vert[i]->ocolor;
				struct ir_operand *var = map[(-st)-1];
				vert[i]->ocolor = ++max;
				vert[i]->color = -1;
				insloadstore(bbs[0], vert[i], st, var, vert, nvert);
				free(vert[i]->shared);
				free(vert[i]->siblings);
				vert[i]->siblings = NULL;
				vert[i]->num_siblings = 0;
				vert[i]->shared = NULL;
				vert[i]->num_shared = 0;
				/*TODO adjust def/kill numbers after removing from list*/
				for (int j = 0; j < nvert; j++) {
					if (j == i) continue;
					if (OVERLAP(vert[i], vert[j])) add_edge(vert[i], vert[j]);
				}
			}
		}
		for (int i = 0; i < nvert; i++) {
			vert[i]->color = -1;
			vert[i]->removed = 0;
		}
		color_graph(vert, nvert, clrs);
		repregs(bbs[0]->blk, vert, nvert);
	} else {
		repregs(bbs[0]->blk, vert, nvert);
	}
	for (int i = 0; i < nmap; i++)
		ir_operand_free(map[i]);
	free(map);
	for (int i = 0; i < nvert; i++)
		vertex_free(vert[i]);
	free(vert);
}

void insloadstore(struct bb *b, struct vertex *v, int stemp, struct ir_operand *rep, struct vertex **vert, int nvert)
{
	struct ir_stmt *cur = b->blk;
	int temp = v->ocolor;
	int s=-1, e=-1;
	int iter = b->ln;
	while (cur) {
		if (cur->arg1 && cur->arg1->type == oper_reg) {
			if (cur->arg1->val.virt_reg == stemp) {
				cur->arg1->val.virt_reg = temp;
				struct ir_stmt *load = ir_stmt_init();
				load->type = stmt_load;
				load->arg1 = copy(rep);
				load->result = from_reg(temp);
				if (cur->prev) cur->prev->next = load;
				cur->prev = load;
				load->next = cur;
				b->len++;
				update_ln(vert, nvert, iter-1, 1);
				if (s==-1) s = iter;
				if (e==-1||iter>e) e = iter;
			}
		}
		if (cur->arg2 && cur->arg2->type == oper_reg) {
			if (cur->arg2->val.virt_reg == stemp) {
				cur->arg2->val.virt_reg = temp;
				struct ir_stmt *load = ir_stmt_init();
				load->type = stmt_load;
				load->arg1 = copy(rep);
				load->result = from_reg(temp);
				if (cur->prev) cur->prev->next = load;
				cur->prev = load;
				load->next = cur;
				b->len++;
				update_ln(vert, nvert, iter-1, 1);
				if (s==-1) s = iter;
				iter++;
				if (e==-1||iter>e) e = iter;
			}
		}
		if (cur->result && cur->result->type == oper_reg) {
			if (cur->result->val.virt_reg == stemp) {
				cur->result->val.virt_reg = temp;
				if (cur->type == stmt_move) {
					cur->type = stmt_store;
					ir_operand_free(cur->result);
					cur->arg2 = cur->arg1;
					cur->arg1 = copy(rep);
					cur->result = NULL;
					//b->len++;
				} else {
					cur->result->val.virt_reg = temp;
					struct ir_stmt *store = ir_stmt_init();
					store->type = stmt_store;
					store->arg1 = copy(rep);
					store->arg2 = from_reg(temp);
					store->next = cur->next;
					if (cur->next) cur->next->prev = store;
					cur->next = store;
					store->prev = cur;
					b->len++;
					update_ln(vert, nvert, iter-1, 1);
					if (s==-1) s = iter;
					if (e==-1||iter>e) e = iter;
				}
			}
		}
		cur = cur->next, iter++;
	}
	v->def = s, v->kill = e;
}

void repregs(struct ir_stmt *cur, struct vertex **vert, int nvert)
{
	while (cur) {
		struct ir_operand *op[3] = { cur->result, cur->arg1, cur->arg2 };
		for (int i = 0; i < 3; i++) {
			if (!op[i] || op[i]->type != oper_reg)
				continue;
			int idx = find_node(vert, nvert, op[i]->val.virt_reg);
			if (idx == -1) continue;
			op[i]->val.virt_reg = vert[idx]->color;
		}
		cur = cur->next;
	}
}

void update_ln(struct vertex **graph, int nvert, int ln, int inc)
{
	for (int i = 0; i < nvert; i++) {
		if (ln > graph[i]->kill) continue;
		graph[i]->kill+=inc?1:-1;
		if (graph[i]->def <= ln)
			graph[i]->def+=inc?1:-1;
	}
}
