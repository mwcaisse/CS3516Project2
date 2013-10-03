all: prog2

prog2: prog2.o
	gcc prog2.o -o prog2
	
prog2.o: prog2.c prog2.h
	gcc -c prog2.c
	
clean:
	rm *.o prog2
