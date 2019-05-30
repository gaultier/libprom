.POSIX:
.SUFFIX:

CFLAGS_COMMON ?= -std=c99 -Weverything -Wno-padded
CFLAGS_RELEASE ?= $(CFLAGS_COMMON) -DNODEBUG -O2 
CFLAGS_DEBUG ?= $(CFLAGS_COMMON) -g -fsanitize=address -O0
DESTDIR ?= /usr/local
DOCKER_IMAGE ?= prom
DOCKER_TAG ?= latest

all: prom_example_release

build: prom_example_release

prom_debug.o: prom.c prom.h
	$(CC) $(CFLAGS_DEBUG) -c prom.c -o $@

prom_release.o: prom.c prom.h
	$(CC) $(CFLAGS_RELEASE) -c prom.c -o $@

libprom_debug.a: prom_debug.o
	$(AR) rvs $@ $^

libprom_release.a: prom_release.o
	$(AR) rvs $@ $^

prom_example_debug: example.c libprom_debug.a
	$(CC) $(CFLAGS_DEBUG) -o $@ $^

prom_example_release: example.c libprom_release.a
	$(CC) $(CFLAGS_RELEASE) -o $@ $^ 

check_debug: prom_example_debug
	(for f in `ls test/*.txt | awk -F '.' '{print $$1}'`; do ./$^ $$f.txt 42 | diff --from-file=$$f.yml - && printf "%s\t\033[32mOK\033[0m\n" $$f || printf "%s\t\033[31mFAIL\033[0m\n" $$f;done;) | column -t;

check_release: prom_example_release
	for f in `ls test/*.txt | awk -F '.' '{print $$1}'`; do ./$^ $$f.txt 42 | diff -q $$f.yml -; done; 

check: check_release

dbuild:
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) .

clean:
	rm -rf *.dSYM *.o *.gch prom_example_debug prom_example_release libprom*.a

install: prom_example_release
	cp prom_example_release $(DESTDIR)/bin/prom_example
	cp libprom_release.a $(DESTDIR)/lib/libprom.a

.PHONY: all build check clean dbuild check check_debug check_release install

