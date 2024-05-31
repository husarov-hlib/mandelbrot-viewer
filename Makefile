cc = gcc
cflags = -g -Wall -lm
objects = main.o color.o

main: $(objects)
	$(cc) $(cflags) -o main $^

%.o: %.c
	$(cc) $(cflags) -c $^

clean:
	rm -f *.o main
