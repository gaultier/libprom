.POSIX:
.SUFFIX:

CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -fPIC -g -O0
DESTDIR ?= /usr/local
DOCKER_IMAGE ?= prom
DOCKER_TAG ?= latest
LDFLAGS ?=  -fsanitize=address
LDLIBS ?= libprom_debug_static.a

.DEFAULT:
all: build

build: libprom_debug_static.a

example: prom_example_debug_static

prom_debug.o: prom.c prom.h
	$(CC) $(CFLAGS) -c prom.c -o $@

libprom_debug_static.a: prom_debug.o
	$(AR) $(ARFLAGS) $@ prom_debug.o

prom_example_debug_static: example.c libprom_debug_static.a
	$(CC) $(CFLAGS) -o $@ example.c $(LDFLAGS) $(LDLIBS)

check: prom_example_debug_static
	(for f in `ls test/*.txt | awk -F. '{print $$1}'`; do  ./prom_example_debug_static  $$f.txt 42 | diff $$f.yml - && printf "%s\t\033[32mOK\033[0m\n" $$f || printf "%s\t\033[31mFAIL\033[0m\n" $$f;done;)

install: libprom_debug_static.a
	cp prom.h $(DESTDIR)/include/prom.h
	echo $(DESTDIR)/include/prom.h > install.txt
	cp libprom_debug_static.a  $(DESTDIR)/lib/libprom_debug_static.a
	ln -s $(DESTDIR)/lib/libprom_debug_static.a $(DESTDIR)/lib/libprom.a
	echo $(DESTDIR)/lib/libprom_debug_static.a >> install.txt
	echo $(DESTDIR)/lib/libprom.a >> install.txt

clean:
	$(RM) -r *.dSYM *.o *.gch prom_example* libprom* example_cxx install.txt

dbuild:
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) --build-arg LIB_TYPE=static .

.PHONY: all build check clean dbuild check install example
