#!/usr/bin/env python2

###
###./perms.py > perms.cpp
###

PP = ["HEAD_S", "HEAD_O", "LOCAL", "CONST"]

def permname(w, x):
	return "perm_" + w + "_" + x

def param(a, s):
	if a == "HEAD_S":
		return "s"
	if a == "HEAD_O":
		return "o"
	if a == "LOCAL":
		return "ITEM(&locals," + s + "i)"
	if a == "CONST":
		return "ITEM(&consts," + s + "i)" #wi, xi

for w in PP:
	for x in PP:
		print "join_gen " + permname(w,x) + "(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)"
		print """{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(""" + param(w, "w") + ", " + param(x, "x") + """)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}"""

print """
void make_perms()
{"""
for x in PP:
	print "permname[" + x + "] = L\"" + x + "\";"

for w in PP:
	for x in PP:
		print "perms[" + w + "][" + x + "] = " + permname(w,x) + ";"

print "}"

