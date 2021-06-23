#!/bin/sh

DIR="`dirname "$0"`"
DIR="`cd ${DIR}; pwd`"

BUILD_ARCH=64
. ${DIR}/thrift-build-base.sh "$@"
