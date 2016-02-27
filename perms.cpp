join_gen perm_HEAD_S_HEAD_S(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
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
join_gen perm_HEAD_S_HEAD_O(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
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
join_gen perm_HEAD_S_LOCAL(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac(s, (&locals[xi]))) {
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
join_gen perm_HEAD_S_CONST(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac(s, (&consts[xi]))) {
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
join_gen perm_HEAD_O_HEAD_S(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
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
join_gen perm_HEAD_O_HEAD_O(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
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
join_gen perm_HEAD_O_LOCAL(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac(o, (&locals[xi]))) {
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
join_gen perm_HEAD_O_CONST(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac(o, (&consts[xi]))) {
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
join_gen perm_LOCAL_HEAD_S(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&locals[wi]), s)) {
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
join_gen perm_LOCAL_HEAD_O(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&locals[wi]), o)) {
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
join_gen perm_LOCAL_LOCAL(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&locals[wi]), (&locals[xi]))) {
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
join_gen perm_LOCAL_CONST(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&locals[wi]), (&consts[xi]))) {
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
join_gen perm_CONST_HEAD_S(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&consts[wi]), s)) {
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
join_gen perm_CONST_HEAD_O(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&consts[wi]), o)) {
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
join_gen perm_CONST_LOCAL(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&consts[wi]), (&locals[xi]))) {
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
join_gen perm_CONST_CONST(nodeid a, join_gen b, pos_t wi, pos_t xi, Locals &consts)
{
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
				
					if ((steps != 0) && (steps % 1000000 == 0))
					dout << "step: " << steps << endl;
					++steps;

				
					//TRACE( dout << sprintPred("a()",a) << endl;)
					
					/*optimization: if we can create a graph of dependencies,
					then not all preds have to be looked up at runtime,
					some can be compiled first and some can have those looked up
					at compilation time*/
					ac = ITEM(preds,a);
					
					//todo assert that access is within bounds?
					while (ac((&consts[wi]), (&consts[xi]))) {
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

void make_perms_table()
{
permname[HEAD_S] = "HEAD_S";
permname[HEAD_O] = "HEAD_O";
permname[LOCAL] = "LOCAL";
permname[CONST] = "CONST";
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
