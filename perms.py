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
	EEE;
	TRC(int call = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(call), ac, bc, &consts]()mutable {
		setproc("join gen");
		return [a, b, wi, xi, entry TRCCAP(call), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc("join");
			TRACE(dout << "call: " << ++call << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred("a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(""" + param(w, "w") + ", " + param(x, "x") + """)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;
				case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					TRACE(dout << "DONE." << endl;)
					END
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

