cc = gcc
cflags = -g -Wall -lm -lSDL2
objects = main.o color.o

main: $(objects)
	$(cc) $(cflags) -o main $^

%.o: %.c
	$(cc) $(cflags) -c $^

clean:
	rm -f *.o main
