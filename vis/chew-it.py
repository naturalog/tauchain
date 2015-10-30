#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json

stepid = -1
toremove = Null

def print_step():
	global stepid
	stepid += 1
	print (json.dumps({"type":"step", "id": stepid}))
	if toremove:
		toremove = Null
		print (json.dumps({"type":"remove", "id":toremove}))
		

print ("""[""")
print_step()

for line in sys.stdin:
	if len(line) == 0:
		print()
	else if line[0] != '{':
		print (json.dumps(line))
	else
		x = json.loads(line)
		if x.type == 'bind':
			step()
			print (json.dumps({"type":"add", "a": x.a, "b": x.b, "id": x.id}))
		if x.type == 'unbind':
			step()
			print (json.dumps({"type":"remove", "id":x.id}))
		if x.type == 'fail':
			step()
			print (json.dumps({"type":"add", "a": x.a, "b": x.b, "id": x.id}))
			toremove = x.id
			 
		



print (""" "end" ])