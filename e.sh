#!/usr/bin/env bash
env ASAN_OPTIONS=detect_leaks=0 LD_PRELOAD=libmarpa/dist/.libs/libmarpa.so  ./tau  --silence addrules --silence readcurly --silence N3 --level 100 run tests/slower/einstein.n3  | less -R
