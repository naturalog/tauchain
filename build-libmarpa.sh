#!/bin/sh

apt-get install  -y build-essential autoconf automake libtool

git submodule init
git submodule update
cd libmarpa/dist
rm ./configure
autoreconf
./configure
make  

