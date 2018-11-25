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

/*Currently just interference between temporaries*/
struct vertex **interference(struct bb *blk, int *num_nodes)
{
	struct vertex **graph = NULL;
	int nvert = 0;

	struct ir_stmt *cur = blk->blk;
	int len = 0;
	int (*use)[2]=NULL, nuse=0;
	/*Create graph vertices, and liveness info*/
	while (cur && len < blk->len) {
		struct ir_operand *op[3] = { cur->result, cur->arg1, cur->arg2 };
		for (int i = 0; i < 3; i++) {
			if (!op[i] || op[i]->type != oper_reg)
				continue;
			/*Do not make interference graph for rN = rN*/
			if (cur->type == stmt_move && op[0] && op[1]) {
				if (op[0]->type == oper_reg && op[1]->type == oper_reg) {
					//if (op[0]->val.virt_reg==op[1]->val.virt_reg)break;
				}
			}
			int tmp = op[i]->val.virt_reg;
			int idx = find_node(graph, nvert, tmp);
			if (!i && idx == -1) {
				graph = add_node(graph, &nvert, tmp);
				nuse++;
				if (!use) use = malloc(sizeof(int)*2);
				else use = realloc(use, sizeof(int)*2*nuse);
				use[nuse-1][0] = len;
				use[nuse-1][1] = len;
			} else if (i && (idx=find_node(graph, nvert, tmp)) != -1) {
				use[idx][1] = len;
			} else if (!op[0] || op[0]->type != oper_reg || op[0]->val.virt_reg != tmp) {
				/*
				graph = add_node(graph, &nvert, tmp);
				nuse++;
				if (!use) use = malloc(sizeof(int)*2);
				else use = realloc(use, sizeof(int)*2*nuse);
				use[nuse-1][0] = 0;
				use[nuse-1][1] = len;*/
			}
		}
		len++;
		cur = cur->next;
	}
	/*For all intersecting ranges, add edge between vertices*/
	for (int i = 0; i < nvert; i++) {
		/*
		printf("R%d:\n", graph[i]->ocolor);
		printf("def'd  %d\n", use[i][0]);
		printf("kill'd %d\n", use[i][1]);
		*/
		for (int j = 0; j < nvert; j++) {
			if (j == i) continue;
			if (use[i][0] < use[j][1] && use[i][1] > use[j][0]) {
				make_edge(graph[i], graph[j]);
			}
		}
	}
	free(use);

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
				/*If memory operand is already in register in bb, use that.
				 * otherwise make a new temporary*/
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
					/*make the move redundant so it will be optimized out*/
					struct ir_stmt *n = cur->prev;
					unlink(cur);
					ir_stmt_free(cur);
					cur = n;
					//cur->arg1 = from_reg(cur->result->val.virt_reg);
					//cur->result=from_reg(-100),cur->arg1=from_reg(-100), cur->arg2=NULL;
					//cur->type = stmt_move;
				} else if (cur->type == stmt_store) {
					/*It is unknown whether store registers are scratch,
					 * so instead of replacing, just change to move*/
					if (cur->arg2 && cur->arg2->type == oper_reg && cur->arg2->val.virt_reg >= 0) {
						int r = cur->arg2->val.virt_reg;
						repstore(cur, r, temp);
						//cur->arg1 = cur->arg2, cur->arg2 = NULL;
						//cur->result = from_reg(r);
						//cur->result=from_reg(-100),cur->arg1=from_reg(-100), cur->arg2=NULL;
						struct ir_stmt *n = cur->prev;
						unlink(cur);
						ir_stmt_free(cur);
						cur = n;
					} else {
						cur->result = from_reg(temp);
						cur->arg1 = cur->arg2, cur->arg2 = NULL;
						cur->type = stmt_move;
					}
					//cur->type = stmt_move;
				}
			}
			cur = cur->next, len++, mem=NULL;
		}
	}
	*num_map = nmap;
	return map;
}

struct vertex **dedup_interference(struct vertex **graph, int *num_vert)
{
	/*Combine duplicate nodes*/
	struct vertex *v1, *v2;
	struct vertex **vert = graph;
	int nvert = *num_vert;
	for (int i = 0; i < nvert; i++) {
		int c = vert[i]->ocolor;
		v1 = vert[i];
		for (int j = 0; j < nvert; j++) {
			if (j == i) continue;
			int c2 = vert[j]->ocolor;
			v2 = vert[j];
			if (c == c2) {
				for (int i = 0; i < v2->num_siblings; i++)
					add_edge(v1, v2->siblings[i]);
				/*
				int os = v1->num_siblings;
				int ns = v1->num_siblings + v2->num_siblings;
				if (!v1->siblings) v1->siblings = malloc(sizeof(*vert)*ns);
				else v1->siblings = realloc(v1->siblings, sizeof(*vert)*ns);
				for (int k = 0; k < v2->num_siblings; k++) {
					v1->siblings[os+k] = v2->siblings[k];
					for (int z = 0; z < v2->siblings[k]->num_siblings; z++) {
						if (v2->siblings[k]->siblings[z] == v2) {
							v2->siblings[k]->siblings[z] = v1;
						}
					}
				}
				v1->num_siblings = ns;
				*/
				if ((j+1) < nvert)
					memmove(vert+j, vert+j+1, (nvert-j-1)*sizeof(*vert));
				nvert--;
				if (!nvert) {
					free(vert);
					vert = NULL;
				}
				else {
					vert = realloc(vert, sizeof(*vert)*nvert);
				}
				j--;
			}
		}
	}
	*num_vert = nvert;
	return vert;
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
		int bnvert = 0;
		struct vertex **bvert = interference(bbs[i], &bnvert);
		int lv = nvert;
		nvert += bnvert;
		if (!vert) vert = malloc(sizeof(*vert)*nvert);
		else vert = realloc(vert, sizeof(*vert)*nvert);
		for (int j = 0; j < bnvert; j++) {
			printf("%d is %d\n", bvert[j]->ocolor, lv+j);
			vert[lv+j] = bvert[j];
		}
		printf("BB INterference %d\n", i);
		/*every output gets the entire basic block as interference*/
		for (int j = 0; j < bbs[i]->nout; j++) {
			struct ir_operand *o = bbs[i]->out[j];
			if (o->type == oper_sym) {
				printf("%s\n", o->val.sym->identifier);
				int oi = temp_map(map, nmap, o->val.sym, o->iter);
				if (!oi) continue;
				int idx = find_node(vert, nvert, oi);

				if (idx < 0) {
					vert = add_node(vert, &nvert, oi);
					idx = nvert-1;
				}
				struct vertex *ov = vert[idx];
				printf("adding edge %d\n", ov->ocolor);
				for (int l = 0; l < bnvert; l++) {
					printf("\t%d\n", bvert[l]->ocolor);
					if (bvert[l] == ov) continue;
					int shared = 0;
					for (int z=0;z<ov->num_shared;z++)
						if (ov->shared[z] == bvert[l]) shared = 1;
					if (shared) continue;
					add_edge(ov, bvert[l]);
				}
			}
		}
		free(bvert);
	}
	vert = dedup_interference(vert, &nvert);
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
	printf("graph {\n");
	for (int i = 0; i < nvert; i++) {
		struct vertex *v = vert[i];
		for (int j = 0; j < v->num_siblings; j++) {
			if (v->siblings[j]->ocolor > v->ocolor) continue;
			printf("\t \"(%d)\" -- \"(%d)\"\n", v->ocolor, v->siblings[j]->ocolor);
		}
	}
	printf("}\n");

	printf("graph {\n");
	for (int i = 0; i < nvert; i++) {
		struct vertex *v = vert[i];
		printf("\t \"(%d)\" -- \"(%d)\"\n", v->ocolor, v->ocolor);
		for (int j = 0; j < v->num_shared; j++) {
			if (v->shared[j]->ocolor > v->ocolor) continue;
			printf("\t \"(%d)\" -- \"(%d)\"\n", v->ocolor, v->shared[j]->ocolor);
		}
	}
	printf("}\n");
	int clrs = 10;
	color_graph(vert, nvert, clrs);
	//return;
	//color_graph(vert, nvert, 6);
	//check for spills
	for (int i = 0; i < nvert; i++) {
		if (vert[i]->color == clrs) {

		}
	}
	struct ir_stmt *cur = bbs[0]->blk;
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
