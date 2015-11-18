#!/bin/sh
test -r libmarpa/dist/.libs/libmarpa.so && exit 0

apt-get update -q
apt-get install -y build-essential autoconf automake libtool

git submodule init
git submodule update
cd libmarpa/dist
rm ./configure
autoreconf
./configure
make  

