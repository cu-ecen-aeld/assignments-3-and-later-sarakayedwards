CFLAGS+= -Wall -Werror

aesdsocket: clean aesdsocket.c

	$(CC) $(CFLAGS) $(INCLUDES) -c aesdsocket.c -o aesdsocket.o
	$(CC) aesdsocket.o $(OBJS) -o aesdsocket $(LDFLAGS)
    
clean:
	rm -f *.o aesdsocket
    
all: aesdsocket
