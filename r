#!/usr/bin/env bash
./perms.py  > perms.cpp
env CC="clang++ -Xclang -fcolor-diagnostics " DBG="-g" make -e "$@"  2>&1 
