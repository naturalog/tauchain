from random import randint, shuffle
import sys

def list(x,q=""):
	a = "( "
	b = bin(x)[2:]
	for i in b:
		a += q+"x" + i + " "
	a += " )"
	return a

min = 2**100000
max = min+5

rules = []

for i in range(max+1,min-1, -1):
	if i == max+1:
		cond = "{ }"
	else:
		cond = "{ " + list(i+1,"?") + " fff fff }"
	rules.append(cond + " => { " + list(i) + " fff fff } . ")

#shuffle(rules)

print ("kb")

for rule in rules:
	print( rule )
	
print("fin.")
print("query")

print ( list(min,"?") + " fff fff .")

print("fin.")