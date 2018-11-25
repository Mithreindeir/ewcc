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

/*Adds an edge if it doesnt already exist*/
void make_edge(struct vertex *v1, struct vertex *v2)
{
	int found = 0;
	for (int i = 0; i < v1->num_siblings; i++) {
		if (v1->siblings[i] == v2) {
			found = 1;
			break;
		}
	}
	if (found) return;
	add_edge(v1, v2);
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

//struct vertex **interference(struct bb *blk, int *num_nodes)
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
		len++;
		cur = cur->next;
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
		/*Iterate through load/store statments and put memory into registers*/
		while (cur && len < b->len) {
			if (cur->type == stmt_store || cur->type == stmt_load) {
				if (cur->arg1 && cur->arg1->type == oper_sym)
					mem = cur->arg1;
			}
			if (mem && !mem->val.sym->spilled) {
				/*Assign reg if not already in one*/
				int temp = temp_map(map, nmap, mem->val.sym, mem->iter);
				/*Add operand to map*/
				if (!temp) {
					/*remove allocation for operand*/
					struct ir_stmt *ac = bbs[0]->blk;
					ac = ac ? ac->next : NULL;
					while (ac && ac->type == stmt_alloc) {
						if (ac->arg1->val.constant == mem->val.sym->size) {
							unlink(ac);
							ir_stmt_free(ac);
							break;
						}
						ac = ac->next;
					}
					nmap++;
					if (!map) map = malloc(sizeof(*map));
					else map = realloc(map, sizeof(*map) * nmap);
					map[nmap-1] = copy(mem);
					temp = ssa--;
				}

				if (cur->type == stmt_load) {
					/*Replace the loaded register with the ssa one*/
					repload(cur, cur->result->val.virt_reg, temp);
					struct ir_stmt *n = cur->prev;
					unlink(cur);
					ir_stmt_free(cur);
					cur = n;
				} else if (cur->type == stmt_store) {
					/*If the store is not with temps, don't replace*/
					if (cur->arg2 && cur->arg2->type == oper_reg && cur->arg2->val.virt_reg >= 0) {
						int r = cur->arg2->val.virt_reg;
						repstore(cur, r, temp);
						struct ir_stmt *n = cur->prev;
						unlink(cur);
						ir_stmt_free(cur);
						cur = n;
					} else {
						cur->result = from_reg(temp);
						ir_operand_free(cur->arg1);
						cur->arg1 = cur->arg2, cur->arg2 = NULL;
						cur->type = stmt_move;
					}
				}
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
	/*Then Basic Block local register graphs are made*/
	struct vertex **vert = NULL;
	int nvert = 0;
	for (int i = 0; i < nbbs; i++) {
		int lvert = nvert;
		vert = interference(bbs[i], vert, &nvert);
		/*Outputs get all of the nodes as edge*/
		for (int j = 0; j < bbs[i]->nout; j++) {
			struct ir_operand *o = bbs[i]->out[j];
			if (o->type != oper_sym) continue;
			int oi = temp_map(map, nmap, o->val.sym, o->iter);
			if (!oi) continue;
			int idx = find_node(vert, nvert, oi);
			if (idx < 0) {
				vert = add_node(vert, &nvert, oi);
				idx = nvert-1;
			}
			struct vertex *ov = vert[idx];
			for (int l = lvert; l < nvert; l++) {
				if (vert[l] == ov) continue;
				if (ov->def > vert[l]->kill) continue;
				add_edge(ov, vert[l]);
			}
		}
	}
	for (int i = 0; i < nvert; i++) {
		printf("R%d:\n", vert[i]->ocolor);
		printf("def'd  %d\n", vert[i]->def);
		printf("kill'd %d\n", vert[i]->kill);
		for (int j = 0; j < nvert; j++) {
			if (j == i) continue;
			if (INTERFERE(vert[i], vert[j])) {
				make_edge(vert[i], vert[j]);
			}
		}
	}

	/*Phi node arguments get coalesced*/
	for (int i = 0; i < nbbs; i++) {
		for (int j = 0; j < bbs[i]->nphi; j++) {
			struct phi *p = &bbs[i]->phi_hdr[j];
			/*The register map is used to find which temp the phi arguments have been mapped too*/
			int r = temp_map(map, nmap, p->var, p->piter);
			if (!r) continue;
			int idx = find_node(vert, nvert, r);
			if (idx < 0) {
				vert = add_node(vert, &nvert, r);
				idx = nvert-1;
			}
			struct vertex *rv = vert[idx];
			for (int l = 0; l < p->niter; l++) {
				int a = temp_map(map, nmap, p->var, p->iters[l]);
				if (!a) continue;
				idx = find_node(vert, nvert, a);
				if (idx < 0) {
					vert = add_node(vert, &nvert, a);
					idx = nvert-1;
				}
				struct vertex *av = vert[idx];
				coalesce(rv, av);
			}
		}
	}
	int clrs = 6;
	color_graph(vert, nvert, clrs);
	repregs(bbs[0]->blk, clrs, vert, nvert);
	for (int i = 0; i < nmap; i++)
		ir_operand_free(map[i]);
	free(map);
	for (int i = 0; i < nvert; i++)
		vertex_free(vert[i]);
	free(vert);
}

void repregs(struct ir_stmt *cur, int clrs, struct vertex **vert, int nvert)
{
	int redo = 0;
	while (cur) {
		struct ir_operand *op[3] = { cur->result, cur->arg1, cur->arg2 };
		for (int i = 0; i < 3; i++) {
			if (!op[i] || op[i]->type != oper_reg)
				continue;
			int idx = find_node(vert, nvert, op[i]->val.virt_reg);
			if (idx == -1) continue;
			if (vert[idx]->color >= clrs) {
				/*spilled*/
				if (vert[idx]->ocolor < 0) {
					/*Was a memory variable to start with*/
					for (int i = 0; i < vert[idx]->num_shared; i++) {

					}
					redo = 1;
				} else {
					/*Scratch temp*/
				}
			} else {
				op[i]->val.virt_reg = vert[idx]->color;
			}
		}
		cur = cur->next;
	}
}
