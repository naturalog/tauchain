#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json, sys

stepid = -1
toremove = None


def pr(x, t):
	print (json.dumps({"type":t, "style":x["style"], "a": x["a"], "b": x["b"]}))


def step():
	global toremove
	print (json.dumps({"type":"step"}))
	print (",")
	if toremove:
		pr(toremove, "remove")
		print (",")
		toremove = None
		

print ("[")

#step()


for line in sys.stdin:
	#print("#" + line)
	if len(line) and line[0] == '{':
		try:
			x = json.loads(line)
		except:
			print ("error:" + str(line))
			raise

		t = x["type"]

		if t in ["kb", "query"]:
		    print(json.dumps(x["value"]))
		else:

			x["style"] = "normal"
			if t == 'bind':
				step()
				pr(x, "add")
			elif t == 'unbind':
				step()
				pr(x, "remove")
			elif t == 'fail':
				step()
				x["style"] = "fail"
				pr(x, "add")
				toremove = x
			else:
				rasars#print (json.dumps(x))
	else:
		print (json.dumps(line[:-1]))
	print (",")
		

print (""" "end" ] """)
