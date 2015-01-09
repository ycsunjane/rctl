all:rctlser

FILE=$(wildcard *.c)
OBJ = $(FILE:.c=.o)

rctlser:$(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

FORCE:          
PHONY += FORCE
.PHONY: $(PHONY)
