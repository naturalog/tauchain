#!/usr/bin/env python2

import datetime
from datetime import datetime as dt
import os
import sys

best = [datetime.timedelta(999999999)] * (len(sys.argv)-1)
runs = 0.0

def z(x):
	return "{:10.8f}".format(x.total_seconds())


def run(c):
        a=dt.utcnow()
        if os.system(c + " > /dev/null") != 0: exit()
        d=dt.utcnow() - a
        return d
        

while True:
	runs += 1

	for i,c in enumerate(sys.argv[1:]):
		r = run(c)
		if r < best[i]: best[i] = r

        print [z(x) for x in best]
        