#!/bin/sh

#echo "$LIB_TYPE"
#echo "$BUILD_TYPE"
#echo "$ASAN"

OS=$(uname)

if [ "$OS" = "Darwin" ]
then
  LIB_DYNAMIC_SUFFIX="dylib"
else
  LIB_DYNAMIC_SUFFIX="so"
fi

if [ "$LIB_TYPE" = "static" ]
then
	LIB_SUFFIX="a"
else
	LIB_SUFFIX="$LIB_DYNAMIC_SUFFIX"
fi

LIB_BASE_NAME="prom_${BUILD_TYPE}_${LIB_TYPE}"

LIB_NAME="lib${LIB_BASE_NAME}.${LIB_SUFFIX}"

CFLAGS_COMMON="-std=c99 -Wall -Wextra -Wpedantic -fPIC"
if [ "$BUILD_TYPE" = "debug" ]
then
    CFLAGS="$CFLAGS_COMMON -g -O0"
else
    CFLAGS="$CFLAGS_COMMON -DNDEBUG -O2"
fi

LIBPROM_OBJECT_FILE="prom_${BUILD_TYPE}.o"

if [ "$LIB_TYPE" = "static" ]
then
    LIB_LINK_CMD="\$(AR) \$(ARFLAGS) \$@ ${LIBPROM_OBJECT_FILE}"
else
    LIB_LINK_CMD="\$(CC) -shared -o \$@ ${LIBPROM_OBJECT_FILE}"
fi

EXAMPLE_NAME="prom_example_${BUILD_TYPE}_${LIB_TYPE}"


if [ "$LIB_TYPE" = "static" ]
then
	EXAMPLE_LDFLAGS=""
	EXAMPLE_LDLIBS="${LIB_NAME}"
else
	EXAMPLE_LDFLAGS="-Xlinker -rpath . -L ."
	EXAMPLE_LDLIBS="-l${LIB_BASE_NAME}"
fi

if [ "$ASAN" = "1" ]
then
	EXAMPLE_LDFLAGS="$EXAMPLE_LDFLAGS -fsanitize=address"
fi

echo ".POSIX:"
echo ".SUFFIX:"
echo ""
echo "CFLAGS ?= ${CFLAGS}"
echo "DESTDIR ?= /usr/local"
echo "DOCKER_IMAGE ?= prom"
echo "DOCKER_TAG ?= latest"
echo "LDFLAGS ?= ${EXAMPLE_LDFLAGS}"
echo "LDLIBS ?= ${EXAMPLE_LDLIBS}"
echo ""
echo ".DEFAULT:"
echo "all: build"
echo ""
echo "build: ${LIB_NAME}"
echo ""
echo "example: ${EXAMPLE_NAME}"
echo ""
echo "${LIBPROM_OBJECT_FILE}: prom.c prom.h"
echo "	\$(CC) \$(CFLAGS) -c prom.c -o \$@"
echo ""
echo "${LIB_NAME}: ${LIBPROM_OBJECT_FILE}"
echo "	${LIB_LINK_CMD}"
echo ""
echo "${EXAMPLE_NAME}: example.c ${LIB_NAME}"
echo "	\$(CC) \$(CFLAGS) -o \$@ example.c \$(LDFLAGS) \$(LDLIBS)"
echo ""
echo "check: ${EXAMPLE_NAME}"
echo '	(for f in `ls test/*.txt | awk -F. '"'"'{print $$1}'"'"'`; do ' "./${EXAMPLE_NAME}" ' $$f.txt 42 | diff $$f.yml - && printf "%s\\t\\033[32mOK\\033[0m\\n" $$f || printf "%s\\t\\033[31mFAIL\\033[0m\\n" $$f;done;)'
echo ""
echo "install: ${LIB_NAME}"
echo "	cp prom.h \$(DESTDIR)/include/prom.h"
echo "	echo \$(DESTDIR)/include/prom.h > install.txt"
echo "	cp ${LIB_NAME}  \$(DESTDIR)/lib/${LIB_NAME}"
echo "	ln -s \$(DESTDIR)/lib/${LIB_NAME} \$(DESTDIR)/lib/libprom.${LIB_SUFFIX}"
echo "	echo \$(DESTDIR)/lib/${LIB_NAME} >> install.txt"
echo "	echo \$(DESTDIR)/lib/libprom.${LIB_SUFFIX} >> install.txt"
echo ""
echo "clean:"
echo "	\$(RM) -r *.dSYM *.o *.gch prom_example* libprom* example_cxx install.txt"
echo ""
echo "dbuild:"
echo "	docker build -t \$(DOCKER_IMAGE):\$(DOCKER_TAG) --build-arg LIB_TYPE=${LIB_TYPE} ."
echo ""
echo ".PHONY: all build check clean dbuild check install example"


