CFLAGS+= -Wall -Werror

test: clean test.c

	$(CC) $(CFLAGS) $(INCLUDES) -c test.c -o test.o
	$(CC) test.o $(OBJS) -o test $(LDFLAGS)
    
clean:
	rm -f *.o test
    
all: test
