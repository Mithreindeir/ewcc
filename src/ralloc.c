#include "ralloc.h"

struct vertex *vertex_init()
{
	struct vertex *v = malloc(sizeof(struct vertex));

	v->siblings = NULL, v->num_siblings = 0;
	v->color = -1, v->ocolor = -1;
	v->removed = 0;

	return v;
}

void add_edge(struct vertex *v, struct vertex *v2)
{
	vertex_add_edge(v, v2);
	vertex_add_edge(v2, v);
}

void vertex_add_edge(struct vertex *v, struct vertex *v2)
{
	v->num_siblings++;
	if (!v->siblings)
		v->siblings = malloc(sizeof(struct vertex *));
	else
		v->siblings =
		    realloc(v->siblings,
			    sizeof(struct vertex *) * v->num_siblings);
	v->siblings[v->num_siblings - 1] = v2;
}

void vertex_free(struct vertex *v)
{
	if (!v)
		return;
	free(v->siblings);
	free(v);
}

int degree(struct vertex *v)
{
	int deg = 0;
	for (int i = 0; i < v->num_siblings; i++)
		deg += !v->siblings[i]->removed;
	return deg;
}

void push(struct vstk *stk, struct vertex *v)
{
	stk->num_vertices++;
	if (!stk->vertices)
		stk->vertices = malloc(sizeof(struct vertex *));
	else
		stk->vertices =
		    realloc(stk->vertices,
			    sizeof(struct vertex *) * stk->num_vertices);
	stk->vertices[stk->num_vertices - 1] = v;
}

struct vertex *pop(struct vstk *stk)
{
	if (!stk->num_vertices)
		return NULL;
	struct vertex *v = stk->vertices[stk->num_vertices - 1];
	stk->num_vertices--;
	if (!stk->num_vertices) {
		free(stk->vertices);
		stk->vertices = NULL;
	} else {
		stk->vertices =
		    realloc(stk->vertices,
			    stk->num_vertices * sizeof(struct vertex *));
	}
	return v;
}

void fill_stack(struct vertex **graph, int num, struct vstk *stk, int k)
{
	if (!k)
		return;
	for (int i = 0; i < num; i++) {
		if (graph[i]->removed)
			continue;
		if (degree(graph[i]) < k) {
			graph[i]->removed = 1;
			push(stk, graph[i]);
			fill_stack(graph, num, stk, k);
			break;
		}
	}
}

void color_graph(struct vertex **graph, int num, int k)
{
	struct vstk stk;
	stk.vertices = NULL;
	stk.num_vertices = 0;
	/*Fill the vertex stack with vertex's in kempe order */
	fill_stack(graph, num, &stk, k);
	/*Array that holds which colors the siblings of a node uses */
	int *used_colors = calloc(sizeof(int), k);

	struct vertex *v;
	while ((v = pop(&stk))) {
		/*skip precolored nodes */
		if (v->color > 0)
			continue;
		int clr = k;
		for (int i = 0; i < k; i++)
			used_colors[i] = 0;
		for (int i = 0; i < v->num_siblings; i++) {
			if (v->siblings[i]->color >= 0
			    && v->siblings[i]->color < k)
				used_colors[v->siblings[i]->color] = 1;
		}
		for (int i = 0; i < k; i++) {
			if (!used_colors[i]) {
				clr = i;
				break;
			}
		}
		if (clr >= k) {	/*spill */
			//v->color = k;
		} else {
			printf("Assigned R%d -> %d\n", v->ocolor, clr);
			v->color = clr;
		}
	}
	/*If k < the degree of all nodes, some nodes may not even be in the stack */
	for (int i = 0; i < num; i++) {
		if (graph[i]->color < 0) {
			printf("Spilled %d\n", graph[i]->ocolor);
		}
	}

	free(stk.vertices);
	free(used_colors);
}
