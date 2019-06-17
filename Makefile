.POSIX:
.SUFFIX:

LIB_TYPE ?= static
BUILD_TYPE ?= release

CFLAGS_COMMON ?= -std=c99 -Weverything -Wno-padded -fPIC
CFLAGS_release ?= $(CFLAGS_COMMON) -DNDEBUG -O2
CFLAGS_debug ?= $(CFLAGS_COMMON) -g -fsanitize=address -O0
DESTDIR ?= /usr/local
DOCKER_IMAGE ?= prom
DOCKER_TAG ?= latest

LIB_SUFFIX_dynamic_MACOS := dylib
LIB_SUFFIX_dynamic_LINUX := so
OS := $(shell uname | awk '/Darwin/ {print "MACOS"} !/Darwin/ {print "LINUX"}')
LIB_SUFFIX_dynamic := $(LIB_SUFFIX_dynamic_$(OS))
LIB_SUFFIX_static := a
LIB_release_static := libprom_release.$(LIB_SUFFIX_static)
LIB_release_dynamic := libprom_release.$(LIB_SUFFIX_dynamic)
LIB_debug_static := libprom_debug.$(LIB_SUFFIX_static)
LIB_debug_dynamic := libprom_debug.$(LIB_SUFFIX_dynamic)
LIB_SUFFIX := $(LIB_SUFFIX_$(LIB_TYPE))
LIB := libprom_$(BUILD_TYPE).$(LIB_SUFFIX)
EXAMPLE := prom_example_$(BUILD_TYPE)_$(LIB_TYPE)

all: build

build: libprom_$(BUILD_TYPE).$(LIB_SUFFIX)

example: $(EXAMPLE)

example_cxx: example_cxx.cpp
	$(CXX) -std=c++17 $< -o $@ -lprom -L .

prom_debug.o: prom.c prom.h
	$(CC) $(CFLAGS_debug) -c prom.c -o $@

prom_release.o: prom.c prom.h
	$(CC) $(CFLAGS_release) -c prom.c -o $@

$(LIB_debug_static): prom_debug.o
	$(AR) $(ARFLAGS) $@ $<

$(LIB_debug_dynamic): prom_debug.o
	$(CC) -shared $< -o $@ -lasan

$(LIB_release_static): prom_release.o
	$(AR) $(ARFLAGS) $@ $<

$(LIB_release_dynamic): prom_release.o
	$(CC) -shared $< -o $@

prom_example_debug_static: example.c $(LIB_debug_static)
	$(CC) $(CFLAGS_debug) -o $@ example.c $(LIB_debug_static)

prom_example_debug_dynamic: example.c $(LIB_debug_dynamic)
	$(CC) $(CFLAGS_debug) -o $@ example.c -Xlinker -rpath . -L . -lprom_$(BUILD_TYPE)

prom_example_release_static: example.c $(LIB_release_static)
	$(CC) $(CFLAGS_release) -o $@ example.c $(LIB_release_static)

prom_example_release_dynamic: example.c $(LIB_release_dynamic)
	$(CC) $(CFLAGS_release) -o $@ example.c -Xlinker -rpath . -L . -lprom_$(BUILD_TYPE)

check: $(EXAMPLE)
	(for f in `ls test/*.txt | awk -F '.' '{print $$1}'`; do ./$(EXAMPLE) $$f.txt 42 | diff $$f.yml - && printf "%s\t\033[32mOK\033[0m\n" $$f || printf "%s\t\033[31mFAIL\033[0m\n" $$f;done;)

dbuild:
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) --build-arg LIB_TYPE=$(LIB_TYPE) .

clean:
	rm -rf *.dSYM *.o *.gch prom_example* libprom* example_cxx

install: $(LIB)
	cp $<  $(DESTDIR)/lib/$<
	ln -s $(DESTDIR)/lib/$< $(DESTDIR)/lib/libprom.$(LIB_SUFFIX)

.PHONY: all build check clean dbuild check install example

