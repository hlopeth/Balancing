CC = g++
run: compile_all
	./main

compile_all:
	$(CC) -o main src/main.cpp
