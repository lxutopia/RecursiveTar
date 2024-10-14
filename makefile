all:
	gcc viewtar.c -o peektar

debug:
	gcc -Wall -g viewtar.c -o peektar

clean:
	rm -f peektar