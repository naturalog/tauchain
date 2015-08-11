#!/bin/sh

git submodule init  
git submodule update  
cd libmarpa-dist;
./configure;
make  

