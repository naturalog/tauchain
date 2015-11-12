import sys, os

#outputString
#The subject is a key and the object is a string, where the strings are to be output in the order of the keys. See cwm --strings in cwm --help.

x = os.system("bash -c ./tau < tests/builtins/outputString | grep AAA")
print x
if x == 123:
	print("test:PASS")
else:
	print("test:FAIL")
