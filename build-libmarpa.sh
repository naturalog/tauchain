#!/bin/sh

sudo apt-get install build-essential autoconf automake libtool

git submodule init;
git submodule update;
cd libmarpa-dist/dist;
rm ./configure;
autoreconf;
./configure;
make  

