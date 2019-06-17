.POSIX:
.SUFFIX:

LIB_TYPE ?= static
BUILD_TYPE ?= release

CFLAGS_COMMON ?= -std=c99 -Weverything -Wno-padded
CFLAGS_Release ?= $(CFLAGS_COMMON) -DNDEBUG -O2
CFLAGS_debug ?= $(CFLAGS_COMMON) -g -fsanitize=address -O0
DESTDIR ?= /usr/local
DOCKER_IMAGE ?= prom
DOCKER_TAG ?= latest
LIB_release_static = libprom_release.a
LIB_release_dynamic = libprom_release.dylib
LIB_debug_static = libprom_debug.a
LIB_debug_dynamic = libprom_debug.dylib

all: prom_example_$(BUILD_TYPE)

build: prom_example_$(BUILD_TYPE)

example_cxx: example_cxx.cpp
	$(CXX) -std=c++17 $^ -o $@ -lprom -L .

prom_debug.o: prom.c prom.h
	$(CC) $(CFLAGS_debug) -c prom.c -o $@

prom_release.o: prom.c prom.h
	$(CC) $(CFLAGS_release) -c prom.c -o $@

libprom_debug.a: prom_debug.o
	$(AR) rvs $@ $^

libprom_release.a: prom_release.o
	$(AR) rvs $@ $^

prom_example_debug: example.c libprom_debug.a
	$(CC) $(CFLAGS_debug) -o $@ $^

prom_example_release: example.c libprom_release.a
	$(CC) $(CFLAGS_release) -o $@ $^ 

check_debug: prom_example_debug
	(for f in `ls test/*.txt | awk -F '.' '{print $$1}'`; do ./$^ $$f.txt 42 | diff --from-file=$$f.yml - && printf "%s\t\033[32mOK\033[0m\n" $$f || printf "%s\t\033[31mFAIL\033[0m\n" $$f;done;) | column -t;

check_release: prom_example_release
	for f in `ls test/*.txt | awk -F '.' '{print $$1}'`; do ./$^ $$f.txt 42 | diff -q $$f.yml -; done; 

check: check_$(BUILD_TYPE)

dbuild:
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) .

clean:
	rm -rf *.dSYM *.o *.gch prom_example* libprom*.a example_cxx

install: prom_example
	cp prom_example_$(BUILD_TYPE) $(DESTDIR)/bin/prom_example_$(BUILD_TYPE)
	cp libprom_$(BUILD_TYPE).a $(DESTDIR)/lib/libprom_$(BUILD_TYPE).a

.PHONY: all build check clean dbuild check check_debug check_release install

