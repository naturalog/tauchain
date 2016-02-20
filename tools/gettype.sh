#!/bin/bash

grep -o -n -d recurse "\(typedef\|class\) .* $1" ./*  
#| grep -o "$.*\.\(cpp\|h\):"
