#!/bin/bash
DIR=$(dirname  $0)
TAU="${DIR}/tau"
LD_PRELOAD=${DIR}/libmarpa/dist/.libs/libmarpa.so $TAU $@
