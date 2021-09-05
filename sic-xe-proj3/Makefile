20171264.out: run.o loader.o assemble.o 20171264.o
	gcc -std=gnu99 -W -Wall run.o loader.o assemble.o 20171264.o -o 20171264.out

run.o: 20171264.h run.h run.c
	gcc -std=gnu99 -c -W -Wall run.c

loader.o: 20171264.h loader.h loader.c
	gcc -std=gnu99 -c -W -Wall loader.c

assemble.o: 20171264.h assemble.h assemble.c
	gcc -std=gnu99 -c -W -Wall assemble.c

20171264.o: 20171264.h 20171264.c
	gcc -std=gnu99 -c -W -Wall 20171264.c

clean:
	rm -f *.o *.out
