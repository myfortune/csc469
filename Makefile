all: experiment_a_1 experiment_a_2

experiment_a_1: experiment_a_1.c tsc.h
	gcc -fgnu89-inline -o experiment_a_1 experiment_a_1.c

experiment_a_2: experiment_a_2.c tsc.h
	gcc -fgnu89-inline -o experiment_a_2 experiment_a_2.c

clean:
	rm experiment_a_1 experiment_a_2 experiment_a_2 *.o
