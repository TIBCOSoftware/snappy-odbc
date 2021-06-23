#!/bin/sh

DIR="`dirname "$0"`"
DIR="`cd ${DIR}; pwd`"

OPTIMIZE=-O2
. ${DIR}/build-conf.sh

GTEST_PREFIX=${SOFTWARE_PREFIX}/googletest-${GOOGLETEST_VERSION}
ARCH_PREFIX=${GTEST_PREFIX}/lin${BUILD_ARCH}

g++ ${CXXFLAGS} -isystem include -I. -pthread -c src/gtest-all.cc src/gtest_main.cc && \
ar -rv libgtest.a gtest-all.o gtest_main.o && \
rm -f gtest-all.o gtest_main.o

rm -rf debug
mkdir debug && \
g++ -g ${CXXFLAGS} -O0 -isystem include -I. -pthread -c src/gtest-all.cc src/gtest_main.cc && \
ar -rv debug/libgtest.a gtest-all.o gtest_main.o && \
rm -f gtest-all.o gtest_main.o

rm -rf ${ARCH_PREFIX}
mkdir -p ${ARCH_PREFIX}/lib/debug && \
cp -r include ${GTEST_PREFIX} && \
cp libgtest.a ${ARCH_PREFIX}/lib && \
cp debug/libgtest.a ${ARCH_PREFIX}/lib/debug
