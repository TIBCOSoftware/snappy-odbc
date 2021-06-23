#!/bin/sh

if [ -z "${THRIFT_VERSION}" ]; then
  THRIFT_VERSION=0.14.1
fi
if [ -z "${BOOST_VERSION}" ]; then
  BOOST_VERSION=1.76.0
fi
if [ -z "${GOOGLETEST_VERSION}" ]; then
  GOOGLETEST_VERSION=1.10.0
fi

if [ -z "$SOFTWARE_PREFIX" ]; then
  SOFTWARE_PREFIX=/gcm/where/software
fi

if [ -z ${BUILD_ARCH} ]; then
  case `uname -m` in
    *64*) BUILD_ARCH="64" ;;
    *) BUILD_ARCH="32" ;;
  esac
fi

BUILD_OUTDIR=
CXXSTD="-std=c++11"
FLAVOR="`lsb_release -isr`"
FLAVOR="`echo ${FLAVOR} | sed 's/ *//g' | tr '[:upper:]' '[:lower:]'`"
case ${FLAVOR} in
  redhat6*|centos6*) BUILD_OUTDIR=rhel6; CXXSTD="-std=c++0x" ;;
  redhat7*|centos7*) BUILD_OUTDIR=rhel7 ;;
  redhat8*|centos8*) BUILD_OUTDIR=rhel8 ;;
  ubuntu14*|ubuntu15*|linuxmint17*) BUILD_OUTDIR=ubuntu14 ;;
  ubuntu16*|ubuntu17*|linuxmint18*) BUILD_OUTDIR=ubuntu16 ;;
  ubuntu18*|ubuntu19*|linuxmint19*) BUILD_OUTDIR=ubuntu18 ;;
  ubuntu20*|ubuntu21*|linuxmint20*) BUILD_OUTDIR=ubuntu20 ;;
  *) BUILD_OUTDIR=linux ;;
esac

if [ -z "${OPTIMIZE}" ]; then
  OPTIMIZE=-O3
fi
GLIB_CFLAGS="${OPTIMIZE} -m${BUILD_ARCH} -DPIC -fPIC -fpermissive"
CFLAGS="${GLIB_CFLAGS}"
CXXFLAGS="${GLIB_CFLAGS} ${CXXSTD}"
GLIB_LDFLAGS="-m${BUILD_ARCH}"
LDFLAGS="${GLIB_LDFLAGS}"

export CFLAGS CXXFLAGS GLIB_CFLAGS LDFLAGS GLIB_LDFLAGS
