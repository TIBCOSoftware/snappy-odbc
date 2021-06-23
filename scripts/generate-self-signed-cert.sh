#!/bin/sh

ALIAS=snappynode
KEYSTORE=keystore.p12
TRUSTPEM=truststore.pem

if [ -n "$1" ]; then
  ALIAS="$1"
fi
if [ -n "$2" ]; then
  KEYSTORE="$2"
fi
if [ -e "$TRUSTPEM" ]; then
  TRUSTPEM=truststore2.pem
fi

keytool -genkey -alias "$ALIAS" -keystore "$KEYSTORE" -keyalg RSA -deststoretype pkcs12
keytool -export -alias "$ALIAS" -keystore "$KEYSTORE" -rfc -file "$TRUSTPEM"
keytool -import -alias "$ALIAS" -file "$TRUSTPEM" -keystore truststore.p12 -deststoretype pkcs12
