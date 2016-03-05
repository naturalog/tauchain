#!/bin/bash
DIR=$(dirname  $0)
TAU="${DIR}/tau    --silence addrules --silence readcurly --silence quad::ctor --silence N3   "
ASAN_OPTIONS="symbolize=1,detect_leaks=0,strict_init_order=1,check_initialization_order=1,verbosity=1" LD_PRELOAD=${DIR}/libmarpa/dist/.libs/libmarpa.so $TAU $@
