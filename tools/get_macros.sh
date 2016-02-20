#!/bin/bash


if [ -e $1 ]; then
	echo "File exists"
else
	grep '^#define.*$' ./* > $1
fi
