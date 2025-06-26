all:
	mkdir -p bin
	mkdir -p objects
	gcc -Wall -Iinclude -c src/common.c -o objects/common.o
	gcc -Wall -Iinclude src/client.c objects/common.o -o bin/client -lm
	gcc -Wall -lpthread -Iinclude src/server.c objects/common.o -o bin/server -lm
