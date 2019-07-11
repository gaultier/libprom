.POSIX:
.SUFFIX:

CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -fPIC -g -O2
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Wpedantic -fPIC -g -O2
DESTDIR ?= /usr/local
DOCKER_IMAGE ?= prom
DOCKER_TAG ?= latest

example: example_c example_cxx

example_c: example.c prom.h
	$(CC) $(CFLAGS) -o $@ example.c

example_cxx: example_cxx.cpp prom.h
	$(CXX) $(CXXFLAGS) -o $@ example_cxx.cpp

check:
	for f in `ls test/*.txt | awk -F. '{print $$1}'`; do  ./example_c $$f.txt 42 | diff $$f.yml - && printf "%s\t\033[32mOK\033[0m\n" $$f || printf "%s\t\033[31mFAIL\033[0m\n" $$f; done;

install: prom.h
	cp prom.h $(DESTDIR)/include/prom.h
	echo $(DESTDIR)/include/prom.h > install.txt

clean:
	rm -rf *.dSYM *.o *.gch example_c example_cxx install.txt

dbuild:
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) --build-arg LIB_TYPE=static .

.PHONY: all check clean dbuild check install example
