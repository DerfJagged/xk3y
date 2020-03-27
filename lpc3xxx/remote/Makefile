CC	= $(CROSS_COMPILE)gcc 
CFLAGS	= -Wall -Ideps/include -ggdb3 -O3
OUT	= remoted
SRCS	= $(wildcard *.c)
OBJS 	= $(SRCS:.c=.o)
LDFLAGS	= -Ldeps/lib -lhid -lusb 

$(OUT): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	$(CROSS_COMPILE)strip $@

clean:
	rm -f $(OUT) *.o

