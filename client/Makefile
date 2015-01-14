all:cmdline.h rctlcli

FILE=$(filter-out cmdline.c, $(wildcard *.c))
OBJ = $(FILE:.c=.o)

cmdline.h:arg.ggo                                                              
	gengetopt < arg.ggo                                                     

rctlcli:cmdline.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

FORCE:          
PHONY += FORCE
.PHONY: $(PHONY)
