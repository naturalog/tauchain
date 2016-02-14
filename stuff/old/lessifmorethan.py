#!/usr/bin/env python
import sys, os
#newin = os.fdopen(sys.stdin.fileno(), 'r', 1)
#sys.stdin = newin
n = 0
lines = []
while True:
    line = sys.stdin.readline()
    if not line: break # EOF
    lines.append(line)
    n += 1
    if (n > int(sys.argv[1])):
	print "meh, calling less ===================================================================================================================================================="
	o = os.popen("less -R", "w")
	print "-----------"
	for l in lines:
	    print "X"
	    o.write(l)
	while True:
	    print "Y"
	    line = sys.stdin.readline()
	    if not line: break # EOF
	    o.write(line)
	exit()

    sys.stdout.write(line)



#i guess our input just decides to not be line-buffered since its connected to a pipe, not terminal...so the output is delayed..:|