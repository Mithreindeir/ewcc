#include <stdio.h>
#include <unistd.h>

#include "frontend/parser.h"
#include "frontend/checker.h"
#include "debug.h"
#include "tac.h"
#include "peep.h"
#include "x86gen.h"
#include "ralloc.h"
#include "live.h"

void test_graph()
{
	int num = 10;
	/*Set up basic graph to color*/
	struct vertex **graph = calloc(num, sizeof(struct vertex*));
	for (int i = 0; i < num; i++) {
		graph[i] = vertex_init();
		graph[i]->ocolor = i;
	}
	/*b c d e f g h j k m*/
	/*0 1 2 3 4 5 6 7 8 9*/
	/*m f e j b k g h d c*/
	/*
Vertex b: color 1: good
Vertex c: color 2: good
Vertex d: color 1: good
Vertex e: color 2: good
Vertex f: color 1: good
Vertex g: color 1: good
Vertex h: color 2: good
Vertex j: color 0: good
Vertex k: color 2: good
Vertex m: color 0: good

	 *
	 * */

	add_edge(graph[0], graph[1]);
	add_edge(graph[0], graph[3]);
	add_edge(graph[0], graph[8]);
	add_edge(graph[0], graph[9]);

	add_edge(graph[1], graph[9]);

	add_edge(graph[2], graph[8]);
	add_edge(graph[2], graph[9]);

	add_edge(graph[3], graph[9]);
	add_edge(graph[3], graph[4]);
	add_edge(graph[3], graph[7]);

	add_edge(graph[4], graph[7]);
	add_edge(graph[4], graph[9]);

	add_edge(graph[5], graph[8]);
	add_edge(graph[5], graph[6]);
	add_edge(graph[5], graph[7]);

	add_edge(graph[6], graph[7]);

	add_edge(graph[7], graph[8]);

	color_graph(graph, num, 3);

	for (int i = 0; i < num; i++)
		printf("Node %d: color %d\n", i, graph[i]->color);

	for (int i = 0; i < num; i++) vertex_free(graph[i]);
	free(graph);
}


int main()
{
	char *str = NULL;
	int slen = 1;
	char buf[64];
	int len = 0;
	while ((len = read(0, buf, 64)) > 0) {
		slen += len;
		if (!str)
			str = malloc(slen);
		else
			str = realloc(str, slen + len);
		memcpy(str + (slen - len - 1), buf, len);
		str[slen - 1] = 0;
	}
	int toks = 0;
	struct token **tok = tokenify(str, &toks);
	free(str);

	struct parser *p = parser_init(tok, toks);
	struct stmt *s = parse_function(p);
	if (s) {
		node_check(s, NULL);
		printf("AST Tree View\n");
		printf("------------------------------------\n");
		node_debug(s);
		printf("\n");
		printf("------------------------------------\n");

		struct ir_stmt *stmt = generate(s);

		optimize(stmt);
		printf("TAC Intermediate Representation\n");
		printf("------------------------------------\n");
		ir_debug(stmt);
		int nbbs=0;
		printf("------------------------------------\n");
		printf("Interference Graph\n");
		printf("------------------------------------\n");
		struct bb **bbs = cfg(stmt, &nbbs);
		bb_creg(bbs[0]);
		optimize(stmt);
		printf("------------------------------------\n");
		printf("After Register Rewriting:\n");
		//instr(stmt);
		ir_debug(stmt);
		for (int i = 0; i < nbbs; i++) {
			bb_free(bbs[i]);
		}
		free(bbs);
		printf("------------------------------------\n");

	       	struct ir_stmt *next=NULL;
		while (stmt) {
			next = stmt->next;
			ir_stmt_free(stmt);
			stmt = next;
		}
		node_free(s);
	}
	parser_free(p);
}
