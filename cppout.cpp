





















#define ultimate
#ifdef CPPOUT
/*
first, simple and naive cppout.


body becomes nested whiles calling predxxx instead of the join-consing thing
lets forget builtins for now
persistent vars needed:
entry headsub headsuc locals
i made some things slightly different than how we do them in our lambdas
	 * instead of returning bools, we indicate being done by setting entry to -1
	 * callee state obviously has to be kept explicitly, not hidden with a lambda
	 * instead of while-looping around an unify coro, theres just an if*/

fstream out;

string predname(nodeid x)
{
	stringstream ss;
	ss << "cpppred" << x;
	return ss.str();
}


string param(PredParam key, pos_t thing_index, pos_t rule_index)
{
	stringstream ss;
	if (key == HEAD_S)
	ss << "s";
	if (key == HEAD_O)
	ss << "o";
	if (key == LOCAL)
	ss << "(&state.locals[" << thing_index << "])";
	if (key == CONST)
	ss << "(&consts" << rule_index << "[" << thing_index << "])";
	return ss.str();
}

nodeid ensure_cppdict(nodeid node)
{
	cppdict[node] = *dict.at(node).value;
	return node;
}


string things_literals(const Locals &things)
{
	stringstream ss;
	ss << "{";
	pos_t i = 0;
	for (Thing t: things) {
		if (is_unbound(t))
			t.node = 0;
		if (i++ != 0) ss << ", ";
		ss << "Thing(" << ThingTypeNames.at(t.type) << ", " << t.node << ")";
		if (is_node(t))
			ensure_cppdict(t.node);
	}
	ss << "}";
	return ss.str();
}


void cppout_pred(string name, vector<Rule> rs)
{
	DBG(out << "/* void cppout_pred */\n";)
	out << "void " << name << "(cpppred_state &state";
	//query? query is baked in for now
	if (name != "query") out << ", Thing *s, Thing *o";
	out << "){\n";
	//for every rule in the kb (not the query) with non-empty body, make an ep-table, static ep_t ep*rule-index*, and for every rule make a const table, Locals const*rule-index*
	for (pos_t i = 0; i < rs.size(); i++) {
		if (rs[i].head && rs[i].body && rs[i].body->size())
			out << "static ep_t ep" << i << ";\n";

		//here we inefficiently do a special round of make_locals just to get consts
		auto &r = rs[i];
		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, r.head, r.body, false);

		out << "static Locals consts" << i << " = " << things_literals(consts) << ";\n";
	}


	if (name == "query")
			out << "static int counter = 0;\n";


	int label = 0;

	out << "switch(state.entry){\n";

	//case 0:
	out << "case "<< label++ << ":\n";

	size_t max_body_len = 0;
	for (auto rule:rs) {
		if (rule.body && max_body_len < rule.body->size())
			max_body_len = rule.body->size();
	}

	out << "state.states.resize(" << max_body_len << ");\n";

	int i = 0;
	//loop over all kb rules for the pred
	for (Rule rule:rs)
	{
		bool has_body = rule.body && rule.body->size();

		out << "//rule " << i << ":\n";
		//out << "// "<<<<":\n";
		out << "case " << label << ":\n";

		label++;
		out << "state.entry = " << label << ";\n";

		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, rule.head, rule.body, false);

		if(locals_template.size())
			out << "state.locals = " << things_literals(locals_template) << ";\n";

		//if it's a kb rule and not the query then we'll
		//make join'd unify-coros for the subject & object of the head
		if (rule.head) {
			pos_t hs, ho; // indexes of head subject and object in locals
			PredParam hsk = find_thing(dict[rule.head->subj], hs, lm, cm);//sets hs
			PredParam hok = find_thing(dict[rule.head->object], ho, lm, cm);

			out << "state.suc = unify(s, " << param(hsk, hs, i) << ");\n";
			out << "if(state.suc()){\n";
			out << "state.ouc = unify(o, " << param(hok, ho, i) << ");\n";
			out << "if(state.ouc()){\n";
		}
		//if it's a kb rule (not the query) with non-empty body, then after the suc/ouc coros succeed, we'll check to see if there's an ep-hit
		if (rule.head && has_body) {
			out << "if (!find_ep(&ep" << i << ", s, o)){\n";
			out << "ep" << i << ".push_back(thingthingpair(s, o));\n";
		}

		//if it's the query or a kb rule with non-empty body: (existing?)		
		if(has_body) {
			size_t j = 0;
			for (pquad bi: *rule.body) {
				out << "//body item" << j << "\n";

				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();

				out << substate << " = cpppred_state();\n";
				out << "do{\n";

				//	check_pred(dict[bi->pred]);

				//set up the subject and object
				pos_t i1, i2;//s and o positions

				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];

				PredParam sk, ok;

				sk = maybe_head(find_thing(s, i1, lm, cm), rule.head, s);
				ok = maybe_head(find_thing(o, i2, lm, cm), rule.head, o);

				if (has(rules, dict[bi->pred]))
					out << predname(dict[bi->pred]) << "(" << substate << ", " <<
						param(sk, i1, i) << ", " << param(ok, i2, i) << ");\n";
				else
					out << substate << ".entry = -1;\n";

				out << "if(" << substate << ".entry == -1) break;\n";
				j++;
			}
		}

		if (name == "query") {
		//would be nice to also write out the head of the rule, and do this for all rules, not just query
			//out << "if (!(counter & 0b11111111111))";
			out << "{dout << \"RESULT \" << counter << \": \";\n";
			ASSERT(rule.body);
			for (pquad bi: *rule.body) {
				pos_t i1, i2;//s and o positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = maybe_head(find_thing(s, i1, lm, cm), rule.head, s);
				ok = maybe_head(find_thing(o, i2, lm, cm), rule.head, o);


				out << "{Thing * bis, * bio;\n";
				out << "bis = getValue(" << param(sk, i1, i) << ");\n";
				out << "bio = getValue(" << param(ok, i2, i) << ");\n";

				out << "Thing n1; if (is_unbound(*bis)) {bis = &n1; n1 = create_node(" << ensure_cppdict(dict[bi->subj]) << ");};\n";
				out << "Thing n2; if (is_unbound(*bio)) {bio = &n2; n2 = create_node(" << ensure_cppdict(dict[bi->object]) << ");};\n";

				out << "dout << str(bis) << \" " << bi->pred->tostring() << " \" << str(bio) << \".\";};\n";
			}
			out << "dout << \"\\n\";}\n";
		}


		if (name == "query")
			out << "counter++;\n";


		if (rule.head && has_body) {
			out << "ASSERT(ep" << i << ".size());\n ep" << i << ".pop_back();\n\n";
		}


		out << "return;\n";
		out << "case " << label++ << ":;\n";
		
		
		if (rule.head && has_body) {
			out << "ep" << i << ".push_back(thingthingpair(s, o));\n";
		}
		
		

		if(rule.body)
			for (pos_t closing = 0; closing < rule.body->size(); closing++)
				out << "}while(true);\n";


		if (rule.head && has_body)
			out << "ASSERT(ep" << i << ".size());\nep" << i << ".pop_back();\n}\n";
			
		if (rule.head) {
			out << "state.ouc();//unbind\n"
					"}\n"
					"state.suc();//unbind\n"
					"}\n";
		}
		i++;
	}
	out << "}state.entry = -1;}\n\n";

}


void yprover::cppout(qdb &goal)
{
	FUN;

	cppdict.clear();
	out.open("out.cpp", fstream::out);
	
	DBG(out << "/* void yprover::cppout */\n";)
	out << "#include \"globals.cpp\"\n";
	out << "#include \"univar.cpp\"\n";
	out << "struct cpppred_state;\n";
	out << "struct cpppred_state {\n" //out << "...""...""...." ? this is treated as one long string
		"int entry=0;\n"
		"vector<Thing> locals;\n"
		"coro suc,ouc;\n"
		"vector<cpppred_state> states;\n};\n"
				   ""
				   ""
				   ;

	out << "/* forward declarations */\n";
	for(auto x: rules) {
		out << "void " << predname(x.first) << "(cpppred_state &state, Thing *s, Thing *o);";
	}


	out << "/* pred function definitions */\n";
	for(auto x: rules) {
		cppout_pred(predname(x.first), x.second);
	}


	auto qit = goal.first.find("@default");
	if (qit == goal.first.end())
		return;

	lists_rules = add_ruleses(rules, quads2rules(goal));
	collect_lists();

	//query is baked in for now
	cppout_pred("query", {Rule(0, qit->second)});



	out << "void cppdict_init(){\n";
	for (auto x:cppdict)
		out << "cppdict[" << x.first << "] = \"" << x.second << "\";\n";
	out << "}\n";


	out << "#include \"cppmain.cpp\"\n" << endl;
	out.close();

}


#endif














/*






-------------------------------------------------------------------------









*/




#ifdef stoopkid




void cppout_consts(string name, vector<Rule> rs)
{
	for (pos_t i = 0; i < rs.size(); i++) {
		auto &r = rs[i];
		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, r.head, r.body, false);
		out << "static Locals consts_" << name << "_" << i << " = " << things_literals(consts) << ";\n";
	}
}


char unify_with_var(Thing * a, Thing * b)
{
    ASSERT(is_unbound(b));
    
    if (!are_equal(*a, *b))
    {
        if (is_unbound(*a))
        {
	    make_this_bound(a, b);
	    return (0b101);
        }
        make_this_bound(b, a);
        return (0b011);
    }
    return (0b001);
}

void unbind_from_var(char magic, Thing * __restrict__ a, Thing * __restrict__ b)
{
    if (magic & 0b100)
	make_this_unbound(a);
    if (magic & 0b010)
	make_this_unbound(b);
}


bool unify_with_const(Thing * a, Thing * b)
{
    ASSERT(!is_bound(a));

    if (are_equal(*a, *b))
	return true;
    if (is_unbound(*a))
    {
	make_this_bound(a, b);
	return true;
    }
    return false;
}

	
void unbind_from_const(Thing *x)
{
        ASSERT(!is_unbound(x));
	if (is_var(*x))
		make_this_unbound(x);
}


//so, were gonna be calling this .
//(with some variations on name and rs)


pquad body_item(pqlist body, int item)
{
	for (auto x: *body)
		if (item-- == 0) return x;
	assert(false);
}


void cppout_pred(string name, vector<Rule> rs,  nodeid arg_pred = 0, int arg_rule = -1, int arg_item = -1)
{
//so..you should try to pass it the name it will use..
	//The names are now specific to the body-items, idk how that affects things like this
	//Need to check this out wrt name:
	//well it will make special constss just for the version..np
	//so..this should work
	stringstream ss;
	string new_name = name;
	ss << predname(arg_pred) << "_" << arg_rule << "_" << arg_item;
	
	out << "//was" << name << "\n";

//	so why not set the name to what it is? so just use ss.str() down there as well?
	if(arg_pred)
		new_name = ss.str(); //ah. let's make it a different var than name or just use ss.str()
	//whats the problem with setting the name ? do we need the old name somewhere? yea we'll be using it
	
	//as i understand it, everything in this function needs the true name
	//i think i spotted a problem down here
	cppout_consts(new_name, rs);

	//out << "static void " << name << "(cpppred_state & __restrict__ state){\n";


	
	out << "static void ";
	if(arg_pred){
		out << predname(arg_pred) << "_" << arg_rule << "_" << arg_item;
	}else{
		out << name;
	}
	out << "(cpppred_state & __restrict__ state){\n";

	

	//For body-items:
	//Should be holding off on any output until after we've determined whether the
	//body-item will match with some rule-head.
	
	if(!arg_pred){
	for (pos_t i = 0; i < rs.size(); i++) {
		if (rs[i].head && rs[i].body && rs[i].body->size())
			out << "static ep_t ep" << i << ";\n";
	}
	}

	size_t max_body_len = 0;
	for (auto rule:rs) {
		if (rule.body && max_body_len < rule.body->size())
			max_body_len = rule.body->size();
	}

	if (name == "cppout_query")
			out << "static int counter = 0;\n";


	if(!arg_pred){

	out << "char uuus;(void)uuus;\n";
	out << "char uuuo;(void)uuuo;\n";
	}
	int label = 0;

	if(!arg_pred){
	out << "switch(state.entry){\n";

	//case 0:
	out << "case "<< label++ << ":\n";

	if(max_body_len){
		out << "state.states.resize(" << max_body_len << ");\n";
	}
	}


	const string PUSH = ".push_back(thingthingpair(state.s, state.o));\n";

	
	bool has_matched = true;
	if(arg_pred){
		 has_matched = false;
	}
	Thing bisthing; 
	Thing biothing;	
	if(arg_pred){
		/*
		vector<Rule> my_rules = rules[arg_pred];
		Rule my_rule = my_rules[arg_rule];
		pqlist my_pbody = my_rule.body;
//		qlist my_body = *my_pbody;
		pquad my_item = body_item(my_pbody, arg_item);
		*/
		
		//it's giving me trouble here
		//i'm trying to get the quad corresponding to the particular body
		//item we're looking at
		pquad q = body_item(rules[arg_pred][arg_rule].body, arg_item);
		
		//well, that should be fixed looks like it :)
		
		if(dict[q->subj] < 0){
			bisthing = create_unbound();
		}else{
			bisthing = create_node(dict[q->subj]);
		}
	
		if(dict[q->object] < 0){
			biothing = create_unbound();
		}else{
			biothing = create_node(dict[q->object]);
		}	
	}	
	


	int i = 0;
	//loop over all kb rules for the pred
	for (Rule rule:rs)
	{
		bool has_body = rule.body && rule.body->size();
		bool subj_op = true;
		bool obj_op = true;


		out << "//rule " << i << ":\n";
		//out << "// "<<<<":\n";
		//out << "case " << label << ":\n";

		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, rule.head, rule.body, false);
		if(!arg_pred){

		if(locals_template.size()){
			out << "state.locals = " << things_literals(locals_template) << ";\n";
		}

		}


		//if it's a kb rule and not the query then we'll
		//make join'd unify-coros for the subject & object of the head
		
		PredParam hsk, hok; //key
		ThingType hst, hot; //type
		pos_t hsi, hoi;     //index
		
		//In the secondary cpp_pred, we'd get some body item as an argument
		//we'd see how it matches with the rule head and:
		//1) If there's a var in the subject and a var in the object of either
		// the rule-head or the body item (and it could be one in one and one in the other), then it's a normal match
		//2) skip checking for matching constants: in the case of
		// (?x a b) with (?y a b), since the objects match, we only unify the subject, and in the case of (a b c) with (a b c), we just go directly into execution of the body rather than doing any head unification
		//3) skip rule execution for non-matching constants:
		//(?y a b) with (?x a c), since the triples can't unify we skip execution of the body (i.e. we don't output it)


		if (rule.head) {

		/*
		Alright here's the fun part: matching & trimming.
		First let's find out if the argument body-item matches the 
		current rule-head.
		On the first matching rule, we'll dump all that output that we've
		previously pushed off until now, now that we know we're going to
		actually do something inside this body-item function, and in general
		we won't output the "state.locals =" line above	until we've actually
		matched with the current rule.	
		If we can't match here, then 'continue' to the next rule without
		outputting anything.


		If we can match, we'll be either outputting the following stuff but
		just for: s/o, s, o, and empty.
		*/

			hsk = find_thing(dict[rule.head->subj], hsi, lm, cm);//sets hs
			hok = find_thing(dict[rule.head->object], hoi, lm, cm);
			hst = get_type(fetch_thing(dict[rule.head->subj  ], locals_template, consts, lm, cm));
			hot = get_type(fetch_thing(dict[rule.head->object], locals_template, consts, lm, cm));
		
		if(arg_pred){
		if(hst == NODE){
			if(get_type(bisthing) == NODE){
				//if the values aren't equal
				if(get_node(consts[hsi]) != get_node(bisthing)){
					continue;
				}else{	
					subj_op = false;
				}	
			}/*else if(get_type(bisthing) == LIST){
				continue;
			}


			*/
		}/*else if(hst == LIST){
			if(get_type(bisthing) == LIST){
				...
			}else if(get_type(bisthing) == NODE){
				continue;
			}

		}

		*/

		if(hot == NODE){
			if(get_type(biothing) == NODE){
				//if the values aren't equal
				if(get_node(consts[hoi]) != get_node(biothing)){
					continue;
				}else{
					obj_op = false;
				}
			}
		}
		
		if(!has_matched){
			has_matched = true;

			for (pos_t i = 0; i < rs.size(); i++) {
				if (rs[i].head && rs[i].body && rs[i].body->size())
					out << "static ep_t ep" << i << ";\n";
			}


			out << "char uuus;(void)uuus;\n";
			out << "char uuuo;(void)uuuo;\n";

			out << "switch(state.entry){\n";

			//case 0:
			out << "case "<< label++ << ":\n";

			if(max_body_len){
				out << "state.states.resize(" << max_body_len << ");\n";
			
			}

		}


		if(locals_template.size()){
			out << "state.locals = " << things_literals(locals_template) << ";\n";
		}
		}
		
			
		
		if(subj_op){	
			if (hst == NODE)
				//might be more issues here, not sure yet
				//well of course, it has to know the right name for the constants
				out << "if (unify_with_const(state.s, " << param(hsk, hsi, new_name, i) << ")){\n";
			else if (hst == UNBOUND)
			{
				out << "uuus = unify_with_var(state.s, " << param(hsk, hsi, new_name, i) << ");\n";
				out << "if (uuus & 1){ state.su.magic = uuus;\n";
			}
			else
			{
				out << "state.su.c = unify(state.s, " << param(hsk, hsi, new_name, i) << ");\n";
				out << "if(state.su.c()){\n";
			}

		}

		if(obj_op){
			if (hot == NODE)
				out << "if (unify_with_const(state.o, " << param(hok, hoi, new_name, i) << ")){\n";
			else if (hot == UNBOUND)
			{
				out << "uuuo = unify_with_var(state.o, " << param(hok, hoi, new_name, i) << ");\n";
				out << "if (uuuo & 1){ state.ou.magic = uuuo;\n";
			}
			else
			{
				out << "state.ou.c = unify(state.o, " << param(hok, hoi, new_name, i) << ");\n";
				out << "if(state.ou.c()){\n";
			}
		}
		
		
		}
		
		
		//if it's a kb rule (not the query) with non-empty body, then after the suc/ouc coros succeed, we'll check to see if there's an ep-hit, hey weren't we supposed to move this outside the head unifications?
		if (rule.head && has_body) {
			out << "if (!cppout_find_ep(&ep" << i << ", state.s, state.o)){\n";
			out << "ep" << i << PUSH;
		}

		out << "state.entry = " << label << ";\n";


		//if it's the query or a kb rule with non-empty body:
		if(has_body) {
			size_t j = 0;
			for (pquad bi: *rule.body) {
				out << "//body item" << j << "\n";

				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();

				out << substate << ".entry = 0;\n";

				//set up the subject and object
				pos_t i1, i2;//s and o positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = find_thing(s, i1, lm, cm);
				ok = find_thing(o, i2, lm, cm);

				//and here perhaps
				out << substate << ".s = getValue(" <<
						param(sk, i1, new_name, i) << ");\n";
				out << substate << ".o = getValue(" <<
						param(ok, i2, new_name, i) << ");\n";

				out << "do{\n";

				//so here's where we'd inline our rules.
				if (has(rules, dict[bi->pred]))
				{
					out << "//call " << predname(dict[bi->pred]) << ";\n";
					if(name != "cppout_query" && dict[bi->pred] == 1966){
						out << name << "_" << i << "_" << j;
					}else{
						out << predname(dict[bi->pred]);
					}
					out << "(" << substate << ");\n";

				}	
				else
					out << substate << ".entry = -1;\n";

				out << "if(" << substate << ".entry == -1) break;\n";
				j++;
			}
		}

		if (name == "cppout_query") {
		//would be nice to also write out the head of the rule, and do this for all rules, not just query
			//out << "if (!(counter & 0b11111111111))";
			out << "{dout << \"RESULT \" << counter << \": \";\n";
			ASSERT(rule.body);
			for (pquad bi: *rule.body) {
				pos_t i1, i2;//s and o positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = find_thing(s, i1, lm, cm);
				ok = find_thing(o, i2, lm, cm);


				out << "{Thing * bis, * bio;\n";
				out << "bis = getValue(" << param(sk, i1, name, i) << ");\n";
				out << "bio = getValue(" << param(ok, i2, name, i) << ");\n";

				out << "Thing n1; if (is_unbound(*bis)) {bis = &n1; n1 = create_node(" << ensure_cppdict(dict[bi->subj]) << ");};\n";
				out << "Thing n2; if (is_unbound(*bio)) {bio = &n2; n2 = create_node(" << ensure_cppdict(dict[bi->object]) << ");};\n";

				out << "dout << str(bis) << \" " << bi->pred->tostring() << " \" << str(bio) << \".\";};\n";
			}
			out << "dout << \"\\n\";}\n";
		}


		if (name == "cppout_query")
			out << "counter++;\n";


		if (rule.head && has_body) {
			out << "ASSERT(ep" << i << ".size());\n ep" << i << ".pop_back();\n\n";
		}


		out << "return;\n";
		out << "case " << label++ << ":;\n";


		if (rule.head && has_body) {
			out << "ep" << i << PUSH;
		}

		if(rule.body)
			for (pos_t closing = 0; closing < rule.body->size(); closing++)
				out << "}while(true);\n";

		if (rule.head && has_body)
			out << "ASSERT(ep" << i << ".size());\nep" << i << ".pop_back();\n}\n";

		if (rule.head) {
			if(obj_op){
			if (hot == NODE)
				out << "unbind_from_const(state.o);\n";
			else if (hot == UNBOUND)
				out << "unbind_from_var(state.ou.magic, state.o, " << param(hok, hoi, name, i) << ");\n";
			else
				out << "state.ou.c();//unbind\n";
			out << "}\n";
			}
		
			if(subj_op){
			if (hst == NODE)
				out << "unbind_from_const(state.s);\n";
			else if (hst == UNBOUND)
				out << "unbind_from_var(state.su.magic, state.s, " << param(hsk, hsi, name, i) << ");\n";
			else
				out << "state.su.c();//unbind\n";
			out << "}\n";
			}
		}
		i++;
	}
	if(has_matched){
	out << "}state.entry = -1;}\n\n";
	}
}


void yprover::cppout(qdb &goal)
{
	FUN;

	cppdict.clear();
	out.open("out.cpp", fstream::out);

	out << "#include \"globals.cpp\"\n";
	out << "#include \"univar.cpp\"\n";
	out << "union unbinder{coro c; char magic; unbinder(){} unbinder(const unbinder&u){} ~unbinder(){}};\n";
	out << "struct cpppred_state;\n";
	out << "struct cpppred_state {\n"
		"int entry=0;\n"
		"vector<Thing> locals;\n"
		"unbinder su,ou;\n"
		"Thing *s, *o;\n"
		"vector<cpppred_state> states;\n};\n"
				   ""
				   ""
				   ;

	out << "/* forward declarations */\n";
	for(auto x: rules) {
		out << "static void " << predname(x.first) << "(cpppred_state &state);";
		
		int i = 0;
		for(auto y: x.second){
			int j = 0;
			//assert(y.body)
			if(y.body)
			for(auto z: *y.body){//for body item?
				out << "static void " << predname(x.first) << "_" << i << "_" << j++ << "(cpppred_state &state);";
			}
			i++;
		}

		
		stringstream ss;
		ss << predname(x.first) << "_unrolled";
		out << "static void " << predname(x.first) << "_unrolled(cpppred_state &state);";

	}


	auto unroll = 0;
	out << "/* pred function definitions */\n";
	for(auto x: rules) {
		cppout_pred(predname(x.first), x.second);
		
		int i = 0;
		for(auto y: x.second){
			int j = 0;
			if(y.body)
			for(auto z: *y.body){
				//note that in general the body-item-function won't
				//be named after the pred that the body-item uses but
				//will instead be named after the pred in the head,
				//with rule & body item numbers (i, j) to specify which
				//body-item in the kb it is.
				
				cppout_pred(predname(dict[z->pred]), rules[dict[z->pred]],x.first,i,j++);
			}
			i++;
		}

		/*
		stringstream ss;
		ss << predname(x.first) << "_unrolled";
		unrolled_cppout_pred(ss.str(), x.second);
		*/
	}


	auto qit = goal.first.find("@default");
	if (qit == goal.first.end())
		return;

	lists_rules = add_ruleses(rules, quads2rules(goal));
	collect_lists();

	//query is baked in for now
	cppout_pred  ("cppout_query", {Rule(0, qit->second)});



	out << "void cppdict_init(){\n";
	for (auto x:cppdict)
		out << "cppdict[" << x.first << "] = \"" << x.second << "\";\n";
	out << "}\n";


	out << "#include \"cppmain.cpp\"\n" << endl;
	out.close();

}



#endif















/*






-------------------------------------------------------------------------









*/




#ifdef unrolled








bool unify_UNBOUND_with_UNBOUND(Thing *a , const Thing *b)
{
	ASSERT(is_unbound(*a));
	make_this_bound(a, b);
	return true;
}
bool unify_UNBOUND_with_NODE(Thing *a , const Thing *b)
{
	ASSERT(is_unbound(*a));
	make_this_bound(a, b);
	return true;
}
bool unify_NODE_with_NODE(Thing *a , const Thing *b)
{
	return are_equal(*a , *b);
}
bool unify_NODE_with_UNBOUND(const Thing *a , Thing *b)
{
	make_this_bound(b, a);
	return true;
}
void unbind_UNBOUND_from_UNBOUND(Thing *a , const Thing *b)
{
	make_this_unbound(a);(void)b;
}
void unbind_UNBOUND_from_NODE(Thing *a , const Thing *b)
{
	make_this_unbound(a);(void)b;
}
void unbind_NODE_from_UNBOUND(const Thing *a , Thing *b)
{
	make_this_unbound(b);(void)a;
}
void unbind_NODE_from_NODE(Thing *a , const Thing *b)
{
	(void)a;(void)b;
}



/*if its a constant its known. if its a var its only known if its its first occurence*/
bool known(ThingType bist, nodeid s, Rule &rule, int j)
{
	bool sknown = true;
						if (bist == UNBOUND)
						{
							if (s == dict[rule.head->subj])
								sknown = false;
							if (s == dict[rule.head->object])
								sknown = false;
							int ccc=0;
							for (pquad mybi: *rule.body) {
								if (j == ccc++) break;
								if (s == dict[mybi->subj])
									sknown = false;
								if (s == dict[mybi->object])
									sknown = false;
							}
						}
						else assert(bist == NODE);
	return sknown;
}




void unrolled_cppout_pred(string name, vector<Rule> rs)
{
	cppout_consts(name, rs);

	out << "static void " << name << "(cpppred_state & __restrict__ state){\n";
	for (pos_t i = 0; i < rs.size(); i++) {
		if (rs[i].head && rs[i].body && rs[i].body->size())
			out << "static ep_t ep" << i << ";\n";
	}

	const string PUSH = ".push_back(thingthingpair(state.s, state.o));\n";


	size_t max_body_len = 0;
	for (auto rule:rs) {
		if (rule.body && max_body_len < rule.body->size())
			max_body_len = rule.body->size();
	}

	if (name == "cppout_query")
			out << "static int counter = 0;\n";


	out << "char uuus;(void)uuus;\n";
	out << "char uuuo;(void)uuuo;\n";

	int label = 0;

	out << "switch(state.entry){\n";

	//case 0:
	out << "case 0:\n";

	if(max_body_len)
		out << "state.states.resize(" << max_body_len << ");\n";
	
	
out << "if(is_unbound(*state.s)) goto UNBOUNDX;"
        "else goto NODEX;"
       "case 1:UNBOUNDX:"
        "if(is_unbound(*state.o)) goto UNBOUNDUNBOUND;"
        "else goto UNBOUNDNODE;"
       "case 2:NODEX:"
        "if(is_unbound(*state.o)) goto NODEUNBOUND;"
        "else goto NODENODE;";

	label = 3;


	
	
	const vector<ThingType> ttt = {UNBOUND, NODE};
	
	for (auto sss: ttt)
	{
	for (auto ooo: ttt)
	{
		out << ThingTypeNames.at(sss) << ThingTypeNames.at(ooo) << ":";
		out << "case " << label++ << ":";
	
		
		
		


	int i = 0;
	//loop over all kb rules for the pred
	for (Rule rule:rs)
	{
		bool has_body = rule.body && rule.body->size();

		out << "//rule " << i << ":\n";
		//out << "// "<<<<":\n";
		


		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, rule.head, rule.body, false);

		if(locals_template.size())
			out << "state.locals = " << things_literals(locals_template) << ";\n";

		//if it's a kb rule and not the query then we'll
		//make join'd unify-coros for the subject & object of the head
		
		PredParam hsk, hok; //key
		ThingType hst, hot; //type
		pos_t hsi, hoi;     //index
		
		if (rule.head) {

			hsk = find_thing(dict[rule.head->subj], hsi, lm, cm);//sets hs
			hok = find_thing(dict[rule.head->object], hoi, lm, cm);
			hst = get_type(fetch_thing(dict[rule.head->subj  ], locals_template, consts, lm, cm));
			hot = get_type(fetch_thing(dict[rule.head->object], locals_template, consts, lm, cm));
			
			
			if (sss != UNBOUND && hst != UNBOUND)
				out << "if (";
			out << "unify_" << ThingTypeNames[sss] << "_with_" << ThingTypeNames[hst];
			out << "(state.s, " << param(hsk, hsi, name, i) << ")";
			if (sss != UNBOUND && hst != UNBOUND)
				out << ")";
			else
				out << ";";
			out << "{\n";
			
			if (ooo != UNBOUND && hot != UNBOUND)
				out << "if (";
			out << "unify_" << ThingTypeNames[ooo] << "_with_" << ThingTypeNames[hot];
			out << "(state.o, " << param(hok, hoi, name, i) << ")";
			if (ooo != UNBOUND && hot != UNBOUND)
				out << ")";
			else
				out << ";";
			out << "{\n";	


		}
		//if it's a kb rule (not the query) with non-empty body, then after the suc/ouc coros succeed, we'll check to see if there's an ep-hit
		if (rule.head && has_body) {
			out << "if (!cppout_find_ep(&ep" << i << ", state.s, state.o)){\n";
			out << "ep" << i << PUSH;
		}

		out << "state.entry = " << label << ";\n";

		//if it's the query or a kb rule with non-empty body: (existing?)
		if(has_body) {
			size_t j = 0;
			for (pquad bi: *rule.body) {
				out << "//body item" << j << "\n";

				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();


				//set up the subject and object
				pos_t i1, i2;//s and o positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = find_thing(s, i1, lm, cm);
				ok = find_thing(o, i2, lm, cm);


				out << substate << ".s = getValue(" <<
						param(sk, i1, name, i) << ");\n";
				out << substate << ".o = getValue(" <<
						param(ok, i2, name, i) << ");\n";






				if (has(rules, dict[bi->pred]))
				{

					
					int label = 0;
					
					
					
				
					bool noinit = true;
					for (auto r: rules[dict[bi->pred]])
						if (r.body && r.body->size())
							noinit = false;
					
					if (noinit)
					{
					

						ThingType bist = get_type(fetch_thing(s, locals_template, consts, lm, cm));
						ThingType biot = get_type(fetch_thing(o, locals_template, consts, lm, cm));
						
						
						bool sknown = known(bist, s, rule, j);
						bool oknown = known(biot, o, rule, j);
						
						int section;
						if (bist == UNBOUND && biot == UNBOUND)
							section = 0;
						if (bist == UNBOUND && biot == NODE)
							section = 1;
						if (bist == NODE && biot == UNBOUND)
							section = 2;
						if (bist == NODE && biot == NODE)
							section = 3;
							
						label = 3+section*(rules[dict[bi->pred]].size()+1);
				
						if(!oknown)
						{
							if (bist == UNBOUND)
								label = 1;
							else label = 2;
						}
						if(!sknown)
							label = 0;
					}


					out << substate << ".entry = " << label << ";\n";
	
					out << "do{\n";
				
				
					out << predname(dict[bi->pred]) << "_unrolled(" << substate << ");\n";
				}
				else
					out << substate << ".entry = -1;\n";

				out << "if(" << substate << ".entry == -1) break;\n";
				j++;
			}
		}

		if (name == "cppout_query") {
		//would be nice to also write out the head of the rule, and do this for all rules, not just query
			//out << "if (!(counter & 0b11111111111))";
			out << "{dout << \"RESULT \" << counter << \": \";\n";
			ASSERT(rule.body);
			for (pquad bi: *rule.body) {
				pos_t i1, i2;//s and o positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = find_thing(s, i1, lm, cm);
				ok = find_thing(o, i2, lm, cm);


				out << "{Thing * bis, * bio;\n";
				out << "bis = getValue(" << param(sk, i1, name, i) << ");\n";
				out << "bio = getValue(" << param(ok, i2, name, i) << ");\n";

				out << "Thing n1; if (is_unbound(*bis)) {bis = &n1; n1 = create_node(" << ensure_cppdict(dict[bi->subj]) << ");};\n";
				out << "Thing n2; if (is_unbound(*bio)) {bio = &n2; n2 = create_node(" << ensure_cppdict(dict[bi->object]) << ");};\n";

				out << "dout << str(bis) << \" " << bi->pred->tostring() << " \" << str(bio) << \".\";};\n";
			}
			out << "dout << \"\\n\";}\n";
		}


		if (name == "cppout_query")
			out << "counter++;\n";


		if (rule.head && has_body) {
			out << "ASSERT(ep" << i << ".size());\n ep" << i << ".pop_back();\n\n";
		}


		out << "return;\n";
		out << "case " << label++ << ":;\n";


		if (rule.head && has_body) {
			out << "ep" << i << PUSH;
		}

		if(rule.body)
			for (pos_t closing = 0; closing < rule.body->size(); closing++)
				out << "}while(true);\n";

		if (rule.head && has_body)
			out << "ASSERT(ep" << i << ".size());\nep" << i << ".pop_back();\n}\n";

		if (rule.head) {
			out << "unbind_" << ThingTypeNames[ooo] << "_from_" << ThingTypeNames[hot];
			out << "(state.o, " << param(hok, hoi, name, i) << " );}\n";
			out << "unbind_" << ThingTypeNames[sss] << "_from_" << ThingTypeNames[hst];
			out << "(state.s, " << param(hsk, hsi, name, i) << " );}\n";
		}
		i++;
	}
	out << "\nstate.entry = -1;return;\n\n";
	
	}}
	
	out << "}}";
	
}
















void yprover::cppout(qdb &goal)
{
	FUN;

	cppdict.clear();
	out.open("out.cpp", fstream::out);

	out << "#include \"globals.cpp\"\n";
	out << "#include \"univar.cpp\"\n";
	out << "union unbinder{coro c; char magic; unbinder(){} unbinder(const unbinder&u){(void)u;} ~unbinder(){}};\n";
	out << "struct cpppred_state;\n";
	out << "struct cpppred_state {\n"
		"int entry=0;\n"
		"unbinder su,ou;\n"
		"Thing *s, *o;\n"
		"vector<Thing> locals;\n"
		"vector<cpppred_state> states;\n};\n"
				   ""
				   "bool silent = false;"
				   ;

	auto unroll = 0;


	out << "/* forward declarations */\n";
	for(auto x: rules) {
		out << preddect(predname(x.first)) << ";\n";
		if(unroll)
		{
		stringstream ss;
		ss << predname(x.first) << "_unrolled";
		out << "static void " << predname(x.first) << "_unrolled(cpppred_state &state);";
		}

	}


	out << "/* pred function definitions */\n";
	for(auto x: rules) {
		cppout_pred(predname(x.first), x.second);
		if(unroll)
		{
		stringstream ss;
		ss << predname(x.first) << "_unrolled";
		unrolled_cppout_pred(ss.str(), x.second);
		}
	}


	auto qit = goal.first.find("@default");
	if (qit == goal.first.end())
		return;

	lists_rules = add_ruleses(rules, quads2rules(goal));
	collect_lists();

	//query is baked in for now
	cppout_pred  ("cppout_query", {Rule(0, qit->second)});



	out << "void cppdict_init(){\n";
	for (auto x:cppdict)
		out << "cppdict[" << x.first << "] = \"" << x.second << "\";\n";
	out << "}\n";


	out << "#include \"cppmain.cpp\"\n" << endl;
	out.close();

}



#endif


/*or rather should be doing hrrm, we should reuse one consts array throughout the pred
unify bm?
s and o re-fetching
http://stackoverflow.com/questions/8019849/labels-as-values-vs-switch-statement
http://www.deadalnix.me/2013/03/23/a-story-about-optimization-llvm-and-the-sentinelinputrange/
http://llvm.org/docs/LangRef.html
















we gotta be making things smaller instead of unrolled


















*/











/*






-------------------------------------------------------------------------









*/




#ifdef ultimate



const static Thing *getValue (const Thing *_x)

	ASSERT(_x);

	const Thing x = *_x;

	// return the value of it's value.
	if (is_bound(x)) {
		//get the pointer
		const Thing * thing = get_thing(x);
		ASSERT(thing);
		//and recurse
		return getValue(thing);
	}

	else
		return _x;
		
	/*may want to reorder this based on usage statistics*/
	
}

static Thing *getValue (Thing *_x)

	ASSERT(_x);

	Thing x = *_x;

	// return the value of it's value.
	if (is_bound(x)) {
		//get the pointer
		Thing * thing = get_thing(x);
		ASSERT(thing);
		//and recurse
		return getValue(thing);
	}

	else
		return _x;
		
	/*may want to reorder this based on usage statistics*/

}


/*getValue makes quite a bit of difference, its a big part of what a rule does*/





bool cppout_would_unify(const Thing *old_, const Thing *now_)
{
	FUN;

	const Thing old = *old_;
	const Thing now = *now_;
	 
	ASSERT(!is_offset(old));
	ASSERT(!is_offset(now));
	ASSERT(!is_bound(now));
	
	if(is_var(old) && is_var(now))
		return true;
	else if (is_node(old))
		return are_equal(old, now);
	else if (is_list(old)) {
		assert(false);
	}
	if ((is_list_bnode(*old_) || is_nil(old_)) && (is_list_bnode(*now_) || is_nil(now_))) {
		assert(false);
	}
	else if (types_differ(old, now)) // in oneword mode doesnt differentiate between bound and unbound!
		return false;
	assert(false);
}




bool cppout_find_ep(const ep_t *ep, const Thing *s, const Thing *o)
{
	FUN;

	ASSERT(!is_offset(*s));
	ASSERT(!is_offset(*o));
	ASSERT(!is_bound(*s));
	ASSERT(!is_bound(*o));

	EPDBG(dout << endl << endl << ep->size() << " ep items." << endl);

	for (auto i: *ep)
	{
		auto os = i.first;
		auto oo = i.second;
		ASSERT(!is_offset(*os));
		ASSERT(!is_offset(*oo));
		//what about !is_bound
		
		//TRACE(dout << endl << " epitem " << str(os) << "    VS     " << str(s) << endl << str(oo) << "    VS    " << str(o) << endl;)
		EPDBG(dout << endl << " epcheck " << str(os) << "    VS     " << str(s) << endl << " epcheck " << str(oo) << "    VS    " << str(o) << endl;)

		if (!cppout_would_unify(os,s) || !cppout_would_unify(oo,o))
		    continue;
		return true;
	}
	return false;
}




fstream out;

string predname(nodeid x)
{
	stringstream ss;
	ss << "cpppred" << x;
	return ss.str();
}


string param(PredParam key, pos_t thing_index, string predname, pos_t rule_index)
{
	stringstream ss;
	if (key == LOCAL)
		ss << "(&state.locals[" << thing_index << "])";
	else if (key == CONST)
		ss << "(&consts_" << predname << "_" << rule_index << "[" << thing_index << "])";
	else assert(false);
	return ss.str();
}

nodeid ensure_cppdict(nodeid node)
{
//	dout << node << endl;
	cppdict[node] = *dict[node].value;
	return node;
}


string things_literals(const Locals &things)
{
	stringstream ss;
	ss << "{";
	pos_t i = 0;
	for (Thing t: things) 
	{
		if (i++ != 0) ss << ", ";

		if (is_node(t))
			ensure_cppdict(get_node(t));

		#ifndef oneword
		if (is_unbound(t))
			t.node = 0;
		ss << "Thing(" << ThingTypeNames.at(t.type) << ", " << t.node << ")";
		#else
		ss << "(Thing)" << t;
		#endif

	}
	ss << "}";
	return ss.str();
}







void cppout_consts(string name, vector<Rule> rs)
{
	for (pos_t i = 0; i < rs.size(); i++) {
		auto &r = rs[i];
		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, r.head, r.body, false);
		out << "static const Locals consts_" << name << "_" << i << " = " << things_literals(consts) << ";\n";
	}
}


char unify_with_var(Thing * a, Thing * b)
{
    ASSERT(is_unbound(*b));
    
    if (!are_equal(*a, *b))
    {
        if (is_unbound(*a))
        {
	    make_this_bound(a, b);
	    return (0b101);
        }
        make_this_bound(b, a);
        return (0b011);
    }
    return (0b001);
}

void unbind_from_var(char magic, Thing * __restrict__ a, Thing * __restrict__ b)
{
    if (magic & 0b100)
	make_this_unbound(a);
    if (magic & 0b010)
	make_this_unbound(b);
}


bool unify_with_const(Thing * a, Thing * b)
{
    ASSERT(!is_bound(*a));

    if (are_equal(*a, *b))
	return true;
    if (is_unbound(*a))
    {
	make_this_bound(a, b);
	return true;
    }
    return false;
}

	
void unbind_from_const(Thing *x)
{
        ASSERT(!is_unbound(*x));
	if (is_var(*x))
		make_this_unbound(x);
}

string preddect(string name)
{
	stringstream ss;
	ss << "static int " << name << "(cpppred_state & __restrict__ state, int entry)";
	return ss.str();
}


string maybe_getval(ThingType t, string what)
{
	stringstream ss;
	bool yes = (t != NODE);
	if (yes)
		ss << "getValue_nooffset(";
	ss << what;
	if (yes)
		ss << ")";
	return ss.str();
}
	

void cppout_pred(string name, vector<Rule> rs)
{
	cppout_consts(name, rs);

	out << "\n" << preddect(name);
	out << "{\n";
	for (pos_t i = 0; i < rs.size(); i++) {
		if (rs[i].head && rs[i].body && rs[i].body->size())
			out << "static ep_t ep" << i << ";\n";
	}

	size_t max_body_len = 0;
	for (auto rule:rs) {
		if (rule.body && max_body_len < rule.body->size())
			max_body_len = rule.body->size();
	}
	
	for (size_t j = 0; j < max_body_len; j++)
		out << "int entry" << j << ";\n";


	if (name == "cppout_query")
			out << "static int counter = 0;\n";



	out << "char uuus;(void)uuus;\n";
	out << "char uuuo;(void)uuuo;\n";

	int label = 0;

	out << "switch(entry){\n";

	//case 0:
	out << "case "<< label++ << ":\n";

	if(max_body_len)
		out << "state.states.resize(" << max_body_len << ");\n";



	const string PUSH = ".push_back(thingthingpair(state.s, state.o));\n";


	int i = 0;
	//loop over all kb rules for the pred
	for (Rule rule:rs)
	{
		bool has_body = rule.body && rule.body->size();

		out << "//rule " << i << ":\n";
		//out << "// "<<<<":\n";
		//out << "case " << label << ":\n";


		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, rule.head, rule.body, false);

		if(locals_template.size())
			out << "state.locals = " << things_literals(locals_template) << ";\n";

		//if it's a kb rule and not the query then we'll
		//make join'd unify-coros for the subject & object of the head
		
		PredParam hsk, hok; //key
		ThingType hst, hot; //type
		pos_t hsi, hoi;     //index
		
		if (rule.head) {

			hsk = find_thing(dict[rule.head->subj], hsi, lm, cm);//sets hs
			hok = find_thing(dict[rule.head->object], hoi, lm, cm);
			hst = get_type(fetch_thing(dict[rule.head->subj  ], locals_template, consts, lm, cm));
			hot = get_type(fetch_thing(dict[rule.head->object], locals_template, consts, lm, cm));
			
			if (hst == NODE)
				out << "if (unify_with_const(state.s, " << param(hsk, hsi, name, i) << ")){\n";
			else if (hst == UNBOUND)
			{
				out << "uuus = unify_with_var(state.s, " << param(hsk, hsi, name, i) << ");\n";
				out << "if (uuus & 1){ state.su.magic = uuus;\n";
			}
			else
			{
				out << "state.su.c = unify(state.s, " << param(hsk, hsi, name, i) << ");\n";
				out << "if(state.su.c()){\n";
			}

			if (hot == NODE)
				out << "if (unify_with_const(state.o, " << param(hok, hoi, name, i) << ")){\n";
			else if (hot == UNBOUND)
			{
				out << "uuuo = unify_with_var(state.o, " << param(hok, hoi, name, i) << ");\n";
				out << "if (uuuo & 1){ state.ou.magic = uuuo;\n";
			}
			else
			{
				out << "state.ou.c = unify(state.o, " << param(hok, hoi, name, i) << ");\n";
				out << "if(state.ou.c()){\n";
			}
		}
		//if it's a kb rule (not the query) with non-empty body, then after the suc/ouc coros succeed, we'll check to see if there's an ep-hit
		if (rule.head && has_body) {
			out << "if (!cppout_find_ep(&ep" << i << ", state.s, state.o)){\n";
			out << "ep" << i << PUSH;
		}

		out << "entry = " << label << ";\n";


		//if it's the query or a kb rule with non-empty body: (existing?)
		if(has_body) {
			size_t j = 0;
			for (pquad bi: *rule.body) {
				out << "//body item" << j << "\n";
				out << "entry" << j << " = 0;\n";

				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();

				

				//set up the subject and object
				pos_t i1, i2; //positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = find_thing(s, i1, lm, cm);
				ok = find_thing(o, i2, lm, cm);
				ThingType bist = get_type(fetch_thing(s, locals_template, consts, lm, cm));
				ThingType biot = get_type(fetch_thing(o, locals_template, consts, lm, cm));


				out << substate << ".s = " << 
					maybe_getval(bist, param(sk, i1, name, i)) << ";\n";
				out << substate << ".o = " << 
					maybe_getval(biot, param(ok, i2, name, i)) << ";\n";

				out << "do{\n";

				if (has(rules, dict[bi->pred]))
					out << "entry" << j << "=" << predname(dict[bi->pred]) << "(" << substate << ", entry" << j << ");\n";
				else
					out << "entry" << j << " = -1;\n";

				out << "if(" << "entry" << j << " == -1) break;\n";
				j++;
			}
		}

		if (name == "cppout_query") {
		//would be nice to also write out the head of the rule, and do this for all rules, not just query
			//out << "if (!(counter & 0b11111111111))";
			out << "{";
			out << "if (!silent) dout << \"RESULT \" << counter << \": \";\n";
			ASSERT(rule.body);
			for (pquad bi: *rule.body) {
				pos_t i1, i2;//s and o positions
				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];
				PredParam sk, ok;
				sk = find_thing(s, i1, lm, cm);
				ok = find_thing(o, i2, lm, cm);


				out << "{Thing * bis, * bio;\n";
				out << "bis = getValue(" << param(sk, i1, name, i) << ");\n";
				out << "bio = getValue(" << param(ok, i2, name, i) << ");\n";

				out << "Thing n1; if (is_unbound(*bis)) {bis = &n1; n1 = create_node(" << ensure_cppdict(dict[bi->subj]) << ");};\n";
				out << "Thing n2; if (is_unbound(*bio)) {bio = &n2; n2 = create_node(" << ensure_cppdict(dict[bi->object]) << ");};\n";

				out << "if (!silent) dout << str(bis) << \" " << bi->pred->tostring() << " \" << str(bio) << \".\";};\n";
			}
			out << "if (!silent) dout << \"\\n\";}\n";
		}
		


		if (name == "cppout_query")
			out << "counter++;\n";


		if (rule.head && has_body) {
			out << "ASSERT(ep" << i << ".size());\n ep" << i << ".pop_back();\n\n";
		}


		/*mm keeping entry on stack for as long as the func is running
		is a good thing, (unless we start jumping into funcs and need to avoid
		any stack state), but we shouldnt save and restore entry's en masse
		around yield, but right after a pred func call returns.
		i think theres anyway not much to be gained from this except the 
		top query function doesnt have to do the stores and loads at all
		..except maybe some memory traffic saving?*/
		if(has_body) {
			size_t j = 0;
			for (pquad bi: *rule.body) {
				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();
				out << substate << ".entry = " << "entry" << j++ << ";\n";
			}
		}


		out << "return entry;\n";
		out << "case " << label++ << ":;\n";
		
		
		if(has_body) {
			size_t j = 0;
			for (pquad bi: *rule.body) {
				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();
				out << "entry" << j++ << " = " << substate << ".entry;\n";
			}
		}


		if (rule.head && has_body) {
			out << "ep" << i << PUSH;
		}

		if(rule.body)
			for (pos_t closing = 0; closing < rule.body->size(); closing++)
				out << "}while(true);\n";

		if (rule.head && has_body)
			out << "ASSERT(ep" << i << ".size());\nep" << i << ".pop_back();\n}\n";

		if (rule.head) {
			if (hot == NODE)
				out << "unbind_from_const(state.o);\n";
			else if (hot == UNBOUND)
				out << "unbind_from_var(state.ou.magic, state.o, " << param(hok, hoi, name, i) << ");\n";
			else
				out << "state.ou.c();//unbind\n";
			out << "}\n";
			if (hst == NODE)
				out << "unbind_from_const(state.s);\n";
			else if (hst == UNBOUND)
				out << "unbind_from_var(state.su.magic, state.s, " << param(hsk, hsi, name, i) << ");\n";
			else
				out << "state.su.c();//unbind\n";
			out << "}\n";
		}
		i++;
	}
	out << "}return -1;}\n\n";
}









void yprover::cppout(qdb &goal)
{
	FUN;

	cppdict.clear();
	out.open("out.cpp", fstream::out);

	out << "#include \"globals.cpp\"\n";
	out << "#include \"univar.cpp\"\n";
	out << "union unbinder{coro c; char magic; unbinder(){} unbinder(const unbinder&u){(void)u;} ~unbinder(){}};\n";
	out << "struct cpppred_state;\n";
	out << "struct cpppred_state {\n"
		"int entry=0;\n"
		"vector<Thing> locals;\n"
		"unbinder su,ou;\n"
		"Thing *s, *o;\n"
		"vector<cpppred_state> states;\n};\n"
				   ""
				   "bool silent = false;"
				   ;

	auto unroll = 0;


	out << "/* forward declarations */\n";
	for(auto x: rules) {
		out << preddect(predname(x.first)) << ";\n";
	}


	out << "/* pred function definitions */\n";
	for(auto x: rules) {
		cppout_pred(predname(x.first), x.second);
	}


	auto qit = goal.first.find("@default");
	if (qit == goal.first.end())
		return;

	lists_rules = add_ruleses(rules, quads2rules(goal));
	collect_lists();

	//query is baked in for now
	cppout_pred  ("cppout_query", {Rule(0, qit->second)});



	out << "void cppdict_init(){\n";
	for (auto x:cppdict)
		out << "cppdict[" << x.first << "] = \"" << x.second << "\";\n";
	out << "}\n";


	out << "#include \"cppmain.cpp\"\n" << endl;
	out.close();

}



#endif


/*




In [4]: #pred1906 size

In [5]: (0x43bf9d - 0x43bc00)/8
Out[5]: 115.625







*/


