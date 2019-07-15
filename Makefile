.POSIX:
.SUFFIX:

FLAGS_COMMON ?= -Wall -Wextra -Wpedantic -fPIC -g -O2
CFLAGS ?= -std=c99 $(FLAGS_COMMON)
CXXFLAGS ?= -std=c++11 $(FLAGS_COMMON)
DESTDIR ?= /usr/local
DOCKER_IMAGE ?= prom
DOCKER_TAG ?= latest

example: example_c example_cxx

example_c: example.c prom.h
	$(CC) $(CFLAGS) -o $@ example.c

example_cxx: example_cxx.cpp prom.h
	$(CXX) $(CXXFLAGS) -o $@ example_cxx.cpp

check:
	for f in test/*.txt; do FILE=$${f%%.*}; ./example_c $$FILE.txt 42 | diff $$FILE.yml - && printf "%s\t\033[32mOK\033[0m\n" $$FILE || printf "%s\t\033[31mFAIL\033[0m\n" $$FILE; done;

install: prom.h
	test -d $(DESTDIR)/include || mkdir -p $(DESTDIR)/include
	cp prom.h $(DESTDIR)/include/prom.h
	echo $(DESTDIR)/include/prom.h > install.txt

clean:
	rm -rf *.dSYM *.o *.gch example_c example_cxx install.txt

dbuild:
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) .

.PHONY: all check clean dbuild check install example
