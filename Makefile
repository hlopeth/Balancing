CC = g++

compile_all:
	$(CC) -o main src/main.cpp

tree:
	./main topology/tree.json
