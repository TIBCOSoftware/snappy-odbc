#!/bin/sh

DIR="`dirname "$0"`"
DIR="`cd ${DIR}; pwd`"

if [ "$2" = "debug" ]; then
  OPTIMIZE="-g -O0"
fi

. ${DIR}/build-conf.sh

THRIFT_PREFIX=${SOFTWARE_PREFIX}/thrift-${THRIFT_VERSION}
if [ -z "${BOOST_PREFIX}" ]; then
  BOOST_PREFIX=${SOFTWARE_PREFIX}/boost-${BOOST_VERSION}
fi

ARCH_PREFIX=${THRIFT_PREFIX}/lin${BUILD_ARCH}
LIBDIR=${ARCH_PREFIX}/lib
PY_PREFIX=${ARCH_PREFIX}/python
PERL_PREFIX=${ARCH_PREFIX}/perl
JAVA_PREFIX=${ARCH_PREFIX}/java

if [ "$2" = "debug" ]; then
  LIBDIR=${LIBDIR}/debug
fi

CXXFLAGS="${CXXFLAGS} -I${BOOST_PREFIX}/include"

export CFLAGS CXXFLAGS GLIB_CFLAGS LDFLAGS GLIB_LDFLAGS PY_PREFIX PERL_PREFIX JAVA_PREFIX

if [ "$1" = "config" ]; then
  make clean
  # 32-bit glib has trouble co-existing with 64-bit so disabling it
  echo ./configure --prefix=${ARCH_PREFIX} --libdir=${LIBDIR} --with-boost=${BOOST_PREFIX} --with-boost-libdir=${BOOST_PREFIX}/lin${BUILD_ARCH} --enable-static --disable-shared --with-pic --disable-plugin --without-php --without-ruby --without-c_glib --without-libevent
  ./configure --prefix=${ARCH_PREFIX} --libdir=${LIBDIR} --with-boost=${BOOST_PREFIX} --with-boost-libdir=${BOOST_PREFIX}/lin${BUILD_ARCH} --enable-static --disable-shared --with-pic --disable-plugin --without-php --without-ruby --without-c_glib --without-libevent
else
  make -j8 || exit 1
  if [ "$1" = "install" ]; then
    make install
  fi
fi
