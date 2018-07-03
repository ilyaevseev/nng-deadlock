#!/usr/bin/gmake -f

ALL_PROGS = nng-deadlock

all: $(ALL_PROGS)

clean:
	/bin/rm -f $(ALL_PROGS)

%: %.c
	gcc -g -Werror $< -lnng -o $@

## END ##
