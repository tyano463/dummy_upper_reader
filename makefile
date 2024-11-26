CC		:= gcc
CFLAGS 	:= -g -O0
#CFLAGS 	:= -O3

OBJS	:= dummy_upper_reader.o
EXE		:= dummy_upper_reader



all:$(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ 

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(EXE)

