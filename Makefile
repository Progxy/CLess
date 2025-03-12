compile: cless.c
	gcc -std=c11 -Wall -Wextra -pedantic -ggdb -lncurses cless.c -o cless

clean:
	rm cless
