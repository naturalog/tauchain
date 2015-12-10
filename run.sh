#!/bin/bash
DIR=$(dirname  $0)
TAU="${DIR}/tau    --silence addrules --silence readcurly --silence quad::ctor --silence N3   "
ASAN_OPTIONS=detect_leaks=0 LD_PRELOAD=${DIR}/libmarpa/dist/.libs/libmarpa.so $TAU $@
