#!/bin/sh

DIR="`dirname "$0"`"
DIR="`cd ${DIR}; pwd`"

if [ "$2" = "debug" ]; then
  OPTIMIZE="-g -O0"
fi

. ${DIR}/build-conf.sh

BOOST_PREFIX=${SOFTWARE_PREFIX}/boost-${BOOST_VERSION}
ARCH_PREFIX=${BOOST_PREFIX}/lin${BUILD_ARCH}
LIBDIR=${ARCH_PREFIX}/lib

VARIANT=release
if [ "$2" = "debug" ]; then
  LIBDIR=${LIBDIR}/debug
  VARIANT=debug
fi

if [ "$1" = "config" ]; then
  ./b2 clean
  rm -rf build bin.v2 b2
  ./bootstrap.sh --prefix=${BOOST_PREFIX} --libdir=${LIBDIR}
else
  ./b2 -j8 --prefix=${BOOST_PREFIX} --libdir=${LIBDIR} --build-dir=build --without-python toolset=gcc variant=${VARIANT} threading=multi link=static address-model=${BUILD_ARCH} install
fi
