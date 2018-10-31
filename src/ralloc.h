#ifndef RALLOC_H
#define RALLOC_H

#include <stdio.h>
#include <stdlib.h>

/*Register Allocation using Kempe's k-node graph coloring algorithm:
 * -Pick vertex on graph with < k degrees
 *  	-If there is none, still do algorithm but last step changes
 * -Remove vertex and add it to stack
 * -Recursive call to Kempes algorithm with k-1
 * -Pop vertex and set it color not used in adjacent node
 *  	-If in first step there was no node with < k degrees:
 *  		-If siblings have duplicate colors, then color is available
 *  		-Leave vertex uncolored, or "spill"
 *
 * Psuedocode:
 * colors = [red, blue, green, ...]
 * stack = []
 * Kempe(graph, k) {
 * 	for vertex in graph {
 * 		if degree(vertex) < k {
 * 			idx = graph.remove(vertex)
 * 			stack.push(vertex)
 * 			Kempe(graph, k-1)
 * 		}
 * 	}
 * 	vertex = stack.pop()
 * 	graph.insert(vertex, idx)
 * 	vertex.color = colors[k]
 * }
 * Vertex's are actually variables, with the siblings being variables whose
 * live ranges intersect with the variables live range*/

/*Right now uses O(n^2) memory usage, will rewrite later*/
struct vertex {
	/*Original IR register to be replaced*/
	int ocolor;
	/*New register*/
	int color;
	/*Removed flag to prevent graph changes*/
	int removed;
	/*Edges are just pointers to vertices*/
	struct vertex **siblings;
	int num_siblings;
};

struct vstk {
	struct vertex **vertices;
	int num_vertices;
};

struct vertex *vertex_init();
void vertex_free(struct vertex *v);
void add_edge(struct vertex *v, struct vertex *v2);
void vertex_add_edge(struct vertex *v, struct vertex *v2);

void push(struct vstk *stk, struct vertex *v);
struct vertex *pop(struct vstk *stk);
int degree(struct vertex *v);

void fill_stack(struct vertex **graph, int num, struct vstk *stk, int k);
/*Given interference graph and number of colors, will color the graph*/
void color_graph(struct vertex **graph, int num, int k);


#endif
