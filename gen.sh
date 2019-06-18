#!/bin/sh

#echo "$LIB_TYPE"
#echo "$BUILD_TYPE"

OS=$(uname)

if [ "$OS" = "Darwin" ]
then
  LIB_SUFFIX="dylib"
else
  LIB_SUFFIX="so"
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

echo ".POSIX:"
echo ".SUFFIX:"
echo ""
echo "CFLAGS ?= ${CFLAGS}"
echo "DESTDIR ?= /usr/local"
echo "DOCKER_IMAGE ?= prom"
echo "DOCKER_TAG ?= latest"
echo ""
echo ".DEFAULT:"
echo "all: build"
echo ""
echo "build: ${LIB}"
echo ""
echo "${LIBPROM_OBJECT_FILE}: prom.c prom.h"
echo "	\$(CC) \$(CFLAGS) -c prom.c -o \$@"
echo ""
echo "${LIB_NAME}: ${LIBPROM_OBJECT_FILE}"
echo "	${LIB_LINK_CMD}"
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
echo "	rm -rf *.dSYM *.o *.gch prom_example* libprom* example_cxx install.txt"
echo ""
echo "dbuild:"
echo "	docker build -t \$(DOCKER_IMAGE):\$(DOCKER_TAG) --build-arg LIB_TYPE=${LIB_TYPE} ."
echo ""
echo ".PHONY: all build check clean dbuild check install example"


