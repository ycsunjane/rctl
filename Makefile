TOPDIR = $(CURDIR)

all: rctlser rctlcli

CC?=gcc
CFLAGS?=-Wall -Wno-unused-function -Wno-unused-value -Wno-unused-variable
CFLAGS+=-I. -I$(TOPDIR)/include
CFLAGS+=-g -DDEBUG
CFLAGS+=-D_GNU_SOURCE
LDFLAGS+=-lpthread -lreadline
STRIP=strip
export CC CFLAGS LDFLAGS STRIP

LIBDIR=$(TOPDIR)/lib
LIB=$(LIBDIR)/rctl.a
export LIB
$(LIB):
	@$(MAKE) -C $(LIBDIR)

SERDIR=$(TOPDIR)/server
rctlser:$(LIB)
	@$(MAKE) -C $(SERDIR)

CLIDIR=$(TOPDIR)/client
rctlcli:$(LIB)
	@$(MAKE) -C $(CLIDIR)

clean:
	-@rm -rf `find . -name "*.o"`
	-@rm -rf `find . -name "*.a"` 
	-@rm -rf $(CLIDIR)/rctlcli $(SERDIR)/rctlser

tags: FORCE
	@find  . -name "*.h" -o -name "*.c" -o -name "*.s" > cscope.files
	@cscope -bkq -i cscope.files
	@ctags -L cscope.files

.PHONY: clean

-include $(SRC:.c=.d)

%.d:cmdline.h %.c
	@set -e; rm -f $@; \
		$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$
