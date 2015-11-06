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
#	if len(line) == 0:
#		print()
	if line[0] == '[':
		json.loads(line)
		print (line)
	elif line[0] == '{':
		try:
			x = json.loads(line)
		except:
			print ("error:" + str(line))
			raise
		t = x["type"]
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
			print (json.dumps(x))
	else:
		print (json.dumps(line))
	print (",")
		

print (""" "end" ] """)
