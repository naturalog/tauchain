#!/usr/bin/env bash
env CC="clang++ -Xclang -fcolor-diagnostics " make -e  2>&1 | less -R -F
