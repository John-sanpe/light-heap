# SPDX-License-Identifier: GPL-2.0-or-later
flags = -g -O2 -Wall -Werror -I src/ -I titer/src/
head  = src/heap.h titer/src/titer.h
obj   = src/heap.o
demo  = examples/benchmark examples/selftest

all: $(demo)

%.o:%.c $(head)
	@ echo -e "  \e[32mCC\e[0m	" $@
	@ gcc -o $@ -c $< $(flags)

$(demo): $(obj) $(addsuffix .c,$(demo))
	@ echo -e "  \e[34mMKELF\e[0m	" $@
	@ gcc -o $@ $@.c $(obj) $(flags)

clean:
	@ rm -f $(obj) $(demo)
