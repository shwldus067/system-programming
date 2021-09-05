20171264.out: assemble.o 20171264.o
	gcc -std=gnu99 -W -Wall assemble.o 20171264.o -o 20171264.out

assemble.o: 20171264.h assemble.h assemble.c
	gcc -std=gnu99 -c -W -Wall assemble.c

20171264.o: 20171264.h 20171264.c
	gcc -std=gnu99 -c -W -Wall 20171264.c

clean:
	rm -f *.o *.out
