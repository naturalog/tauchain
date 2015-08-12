#!/bin/sh

sudo apt-get install build-essentials autoconf automake libtool

git submodule init;
git submodule update;
cd libmarpa-dist/dist;
./configure;
make  

