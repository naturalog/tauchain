join_gen perm_HEAD_S_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(s, s)) {
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
}
join_gen perm_HEAD_S_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(s, o)) {
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
}
join_gen perm_HEAD_S_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(s, ITEM(&locals,xi))) {
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
}
join_gen perm_HEAD_S_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(s, ITEM(&consts,xi))) {
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
}
join_gen perm_HEAD_O_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(o, s)) {
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
}
join_gen perm_HEAD_O_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(o, o)) {
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
}
join_gen perm_HEAD_O_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(o, ITEM(&locals,xi))) {
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
}
join_gen perm_HEAD_O_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(o, ITEM(&consts,xi))) {
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
}
join_gen perm_LOCAL_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&locals,wi), s)) {
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
}
join_gen perm_LOCAL_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&locals,wi), o)) {
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
}
join_gen perm_LOCAL_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&locals,wi), ITEM(&locals,xi))) {
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
}
join_gen perm_LOCAL_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&locals,wi), ITEM(&consts,xi))) {
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
}
join_gen perm_CONST_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&consts,wi), s)) {
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
}
join_gen perm_CONST_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&consts,wi), o)) {
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
}
join_gen perm_CONST_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&consts,wi), ITEM(&locals,xi))) {
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
}
join_gen perm_CONST_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	EEE;
	TRC(int round = 0;)
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry TRCCAP(round), ac, bc, &consts](Thing *s, Thing *o, Thing *locals)mutable {
			setproc(L"join");
			TRACE(dout << "round: " << ++round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = ITEM(preds,a);
					while (ac(ITEM(&consts,wi), ITEM(&consts,xi))) {
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
}

void make_perms()
{
permname[HEAD_S] = L"HEAD_S";
permname[HEAD_O] = L"HEAD_O";
permname[LOCAL] = L"LOCAL";
permname[CONST] = L"CONST";
perms[HEAD_S][HEAD_S] = perm_HEAD_S_HEAD_S;
perms[HEAD_S][HEAD_O] = perm_HEAD_S_HEAD_O;
perms[HEAD_S][LOCAL] = perm_HEAD_S_LOCAL;
perms[HEAD_S][CONST] = perm_HEAD_S_CONST;
perms[HEAD_O][HEAD_S] = perm_HEAD_O_HEAD_S;
perms[HEAD_O][HEAD_O] = perm_HEAD_O_HEAD_O;
perms[HEAD_O][LOCAL] = perm_HEAD_O_LOCAL;
perms[HEAD_O][CONST] = perm_HEAD_O_CONST;
perms[LOCAL][HEAD_S] = perm_LOCAL_HEAD_S;
perms[LOCAL][HEAD_O] = perm_LOCAL_HEAD_O;
perms[LOCAL][LOCAL] = perm_LOCAL_LOCAL;
perms[LOCAL][CONST] = perm_LOCAL_CONST;
perms[CONST][HEAD_S] = perm_CONST_HEAD_S;
perms[CONST][HEAD_O] = perm_CONST_HEAD_O;
perms[CONST][LOCAL] = perm_CONST_LOCAL;
perms[CONST][CONST] = perm_CONST_CONST;
}
