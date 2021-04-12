CC = g++

compile_graph:
	$(CC) -o graph_main src/graph_main.cpp

compile_tree:
	$(CC) -o tree_main src/tree_main.cpp

tree: compile_tree
	./tree_main topology/tree.json

graph: compile_graph
	./graph_main topology/simple_graph.json
