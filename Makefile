all: server client

CC?=gcc
CFLAGS?=-Wall -Wno-unused-function -Wno-unused-value -Wno-unused-variable
CFLAGS+=-I. 
CFLAGS+=-g -DDEBUG
LIB+=-lpthread
STRIP=strip

SERSRC=common.c ser.c

server: $(SERSRC)
	$(CC) $(CFLAGS) -DSERVER $(LIB) $^ -o $@

CLISRC=common.c rctl.c cli.c

client: $(CLISRC)
	$(CC) $(CFLAGS) -DCLIENT $^ -o $@

clean:
	-@rm -rf client server *.o *.d

.PHONY: clean

-include $(SRC:.c=.d)

%.d:cmdline.h %.c
	@set -e; rm -f $@; \
		$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$
