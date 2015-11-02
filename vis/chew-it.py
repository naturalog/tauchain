#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json, sys

stepid = -1
toremove = None

def step():
	global stepid, toremove
	stepid += 1
	print (json.dumps({"type":"step", "id": stepid},))
	print (",")
	if toremove:
		print (json.dumps({"type":"remove", "a": x.a, "b": x.b}))
		print (",")
		toremove = None
		

print ("[")

step()

for line in sys.stdin:
	if len(line) == 0:
		print()
	elif line[0] != '{':
		print (json.dumps(line))
		print (",")
	else:
		try:
			x = json.loads(line)
		except:
			print ("error:" + str(line))
			raise
		if x.type == 'bind':
			step()
			print (json.dumps({"type":"add", "a": x.a, "b": x.b}))
			print (",")
		if x.type == 'unbind':
			step()
			print (json.dumps({"type":"remove", "a": x.a, "b": x.b}))
			print (",")
		if x.type == 'fail':
			step()
			print (json.dumps({"type":"add", "a": x.a, "b": x.b}))
			print (",")
			toremove = x
		else:
			print (json.dumps(x))
			print (",")
		

print (""" "end" ] """)
