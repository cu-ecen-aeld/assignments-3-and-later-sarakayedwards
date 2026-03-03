writer: clean writer.c

	$(CC) $(CFLAGS) $(INCLUDES) -c writer.c -o writer.o 
	$(CC) writer.o $(OBJS) -o writer $(LDFLAGS)
	

clean:
	rm -f *.o writer
	
all: writer
