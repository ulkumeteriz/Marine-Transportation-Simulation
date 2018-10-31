all:
	gcc simulator.c cargoLL.c queue.c writeOutput.c -lpthread -o simulator
run:
	./simulator