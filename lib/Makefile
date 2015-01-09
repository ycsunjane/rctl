all:$(LIB)

FILE = $(wildcard *.c)
OBJ=$(FILE:.c=.o)
$(LIB):$(OBJ)
	$(AR) r $(notdir $@) $^

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

FORCE:          
PHONY += FORCE
.PHONY: $(PHONY)
