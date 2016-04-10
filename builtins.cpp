#ifdef DHT

#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::lock
#include <opendht.h>

string strhash(string x)
{
	std::hash<std::string> fn;
	size_t h = fn(x);
	stringstream sss;
	sss << /*std::showbase << std::uppercase << std::hex <<*/ h;// "0xbab5";//h;
	return sss.str();
}



/**
 * Print va_list to std::ostream (used for logging).
 */
void
printDhtLog(std::ostream& s, char const* m, va_list args) {
    static constexpr int BUF_SZ = 8192;
    char buffer[BUF_SZ];
    int ret = vsnprintf(buffer, sizeof(buffer), m, args);
    if (ret < 0)
        return;
    s.write(buffer, std::min(ret, BUF_SZ));
    if (ret >= BUF_SZ)
        s << "[[TRUNCATED]]";
    s.put('\n');
}

void
enableDhtLogging(dht::DhtRunner& dht)
{
    dht.setLoggers(
        [](char const* m, va_list args){ dout << "ERR"; printDhtLog(dout, m, args);  },
        [](char const* m, va_list args){ dout << "WARN"; printDhtLog(dout, m, args); },
        [](char const* m, va_list args){ printDhtLog(dout, m, args); }
    );
}



#endif


void build_in_dht()
{
#ifdef DHT

	static bool running = false;
	static mutex mut;
	static dht::DhtRunner ht;
	
	
	if (!running)
	{
		running = true;
		int port = 4443;
		dout << "firing up DHT on port " << port << endl;
		// Launch a dht node on a new thread, using a
		// generated RSA key pair, and listen on port 4222.
		ht.run(port, dht::crypto::generateIdentity(), true);

		// Join the network through any running node,
		// here using a known bootstrap node.
		ht.bootstrap("127.0.0.1", "4444");

		// put some data on the dht
		std::vector<uint8_t> some_data(5, 10);
		ht.put("unique_key", some_data);

		// put some data on the dht, signed with our generated private key
		ht.putSigned("unique_key_42", some_data);

	}

	auto ht_ = &ht;

	EEE;	
	
	string bu = "http://idni.org/dht#put";
	auto bui = dict.set(mkiri(pstr(bu)));

	builtins[bui].push_back(
		[bu, entry, ht_](Thing *dummy, Thing *x) mutable {
		setproc(bu);
		TRACE_ENTRY;
		dout <<"sssss" << endl;
		switch(entry){
		case 0:
			x = getValue(x);
			
			if(is_node(*x))
			{
				node n = dict[get_node(*x)];
				string v = *n.value;
				string key,val;

				if (n._type == node::IRI)
				{
					if (has(mykb->first, v))
					{
						key = "root_graph_" + v;
						stringstream ss;
						ss << mykb->first[v];
						val = ss.str();
					}
					else
					{
						string h = strhash(v);
						key = "root_iri_" + h;
						val = v;
					}
				}
				else if (n._type == node::LITERAL)
				{
					string h = strhash(v);
					key = "root_lit_" + h;
					val = v;
				}
				else
				{
					dout << "nope." << endl;
					DONE;
				}
				dout << "putting " << key << "=" << val << endl;
				ht_->put(key, val);
				
			}
			else
				dout << "nope." << endl;
			
						
			END;
		}
	});
	
	
	
	bu = "http://idni.org/dht#dbg";
		     http://idni.org/dht#dbg
	bui = dict.set(mkiri(pstr(bu)));

	builtins[bui].push_back(
		[bu, entry, ht_](Thing *dummy, Thing *x) mutable {
		setproc(bu);
		TRACE_ENTRY;
		
		switch(entry){
		case 0:
			x = getValue(x);
					
			if(is_node(*x))
			{
				node n = dict[get_node(*x)];
				string v = *n.value;
				if (v == "on")
				{
					MSG("dht dbg on");
					enableDhtLogging(*ht_);
				}
				else{
					MSG("dht dbg off");
					ht_->setLoggers(dht::NOLOG, dht::NOLOG, dht::NOLOG);
				}
			}
						
			END;
		}
	});


    // get data from the dht
    ht_->get("other_unique_key", [](const std::vector<std::shared_ptr<dht::Value>>& values) {
        // Callback called when values are found
        for (const auto& value : values)
            dout << "Found value: " << *value << std::endl;
        return true; // return false to stop the search
    });
#endif

}





//Outer vector: list
//Inner vector: triples of nodeids
void add_facts(vector<vector<nodeid>> facts)
{
	//std::map<nodeid,std::vector<std::pair<Thing*,Thing*>>>
	ths_t &ths = *new ths_t;
	ths_garbage = &ths;///.push_back(ths);

	//Map each pred to a list of it's subject/object pairs
	for (auto f: facts)
		ths[f[1]].push_back({
			create_node(f[0]),
			create_node(f[2])});
	
	coro suc, ouc;
	//For each pred in the ths:
	//std::pair<nodeid,std::vector<std::pair<Thing*,Thing*>>>
	for (auto ff:ths)
	{
		//std::vector<std::pair<Thing*, Thing*>>
		auto &pairs = ff.second;
		EEE; //char entry = 0;
		pos_t  pi = 0;

		//Map each pred to a coro on it's subject/object
		builtins[ff.first].push_back([suc,ouc,pi,pairs,entry](Thing *s_, Thing *o_)	mutable{
			switch(entry)
			{
			case 0:
				//hrmm.. where is pi being incremented? looks 
				//like this just does pi=0 then exits.
				/*yea looks like you found a bug
this thing looks pretty suboptimal btw...buti guess it should get the job done*/
				if (pi < pairs.size())//fixed?
				{			
				//Generate a unify coro for the subject; 
				//on the fly.
				//Then run it.
					suc = unify(s_,&pairs[pi].first);
					while(suc()){
					//Again for the object
						ouc = unify(o_,&pairs[pi].second);
						while(ouc())
						{
						//If both succeed then yield
						//true.
						//not quite
							entry = LAST;
							

							return true;
				/*
				entry = LAST;
				 so...wanna fix this?
			*/
			case_LAST:;
						} 
					}
				}
			END;
			}
		});
	} 
}
	



/*
 * RDF - http://www.w3.org/TR/lbase/#using - where the ?y(?x) thing comes from
 */


void build_in_facts()
{

//Is it beneficial to add the redundant facts or not?
	add_facts({

//These are redundant given the rule {?x rdf:type rdfs:Class} => {?x rdfs:subClassOf rdfs:Resource}
//rdfsResource rdfssubClassOf rdfsResource
//rdfsClass rdfssubClassOf rdfsResource
//rdfsLiteral rdfssubClassOf rdfsResource

		{rdfsDatatype,                    rdfssubClassOf,    rdfsClass},
//These are redundant given the rule {?x rdf:type rdfs:Datatype} => {?x rdfs:subClassOf rdfs:Literal}
//rdflangString rdfssubClassOf rdfsLiteral	//(redundant)
//rdfHTML rdfssubClassOf rdfsLiteral		//(redundant)
		{rdfXMLLiteral,                   rdfssubClassOf,    rdfsLiteral}, //(redundant)
//rdfProperty rdfssubClassOf rdfsResource	//(redundant)
//rdfsContainer rdfssubClassOf rdfsResource	//(redundant)
		{rdfAlt,                          rdfssubClassOf,    rdfsContainer},
		{rdfBag,                          rdfssubClassOf,    rdfsContainer},
		{rdfsContainerMembershipProperty, rdfssubClassOf,    rdfProperty},
		{rdfsDatatype,                    rdfssubClassOf,    rdfsClass},
		{rdfSeq,                          rdfssubClassOf,    rdfsContainer},


		{rdfsisDefinedBy,                 rdfssubPropertyOf, rdfsseeAlso},




		//I think we need these:
		
		{rdfsResource, rdftype, rdfsClass},
		{rdfsClass, rdftype, rdfsClass},
		{rdfsLiteral, rdftype, rdfsClass},
		{rdfsDatatype, rdftype, rdfsClass},
		
	
		
		{rdflangString, 		rdftype, 		rdfsDatatype},
		{rdfHTML, 			rdftype, 		rdfsDatatype},
		{rdfXMLLiteral,                 rdftype,        	rdfsDatatype},

		//I think we need this one:
		{rdfProperty, rdftype, rdfsClass},



//possibly redundant via their usage in rdfsdomain & rdfsrange (not sure):
//rdfsrange rdftype rdfProperty
//rdfsdomain rdftype rdfProperty
//rdftype rdftype rdfProperty
//rdfssubClassOf rdftype rdfProperty
//rdfssubPropertyOf rdftype rdfProperty
//rdfslabel rdftype rdfProperty
//rdfscomment rdftype rdfProperty

		{rdfsContainer, 	rdftype, 	rdfsClass},




//rdfBag rdftype rdfsClass
//rdfSeq rdftype rdfsClass
//rdfAlt rdftype rdfsClass
//redundant due to:
/*
	[rdfBag, rdfSeq, rdfAlt] rdfssubClassOf rdfsContainer
	rdfssubClassOf rdfsdomain rdfsClass
	{?p rdfsdomain ?c. ?s ?p ?o} => {?s rdf:type ?c}
*/





//{rdfsContainerMembershipProperty, rdftype, rdfsClass},
//redundant due to: 
/*
	rdfsContainerMembershipProperty rdfssubClassOf rdfProperty
	rdfssubClassOf rdfsdomain rdfsClass
	{?p rdfsdomain ?c. ?s ?p ?o} => {?s rdf:type ?c}
*/




//rdf:_1 rdftype rdfsContainerMembershipProperty
//rdf:_2 rdftype rdfsContainerMembershipProperty
//rdf:_3 rdftype rdfsContainerMembershipProperty
//...

//rdfsmember rdftype rdfProperty (possibly redundant due to domain & range)

		{rdfList, 			rdftype,	rdfsClass},
		{rdffirst,                        rdftype,           owlFunctionalProperty},//?
		{rdfrest,                         rdftype,           owlFunctionalProperty},
		{rdfnil,                          rdftype,           rdfList},


		{rdfStatement, 		rdftype, 	rdfsClass},

//possibly redundant due to domain & range:
//rdfsubject rdftype rdfProperty
//rdfpredicate rdftype rdfProperty
//rdfobject rdftype rdfProperty

//possibly redundant due to domain & range:
//rdfsseeAlso rdftype rdfProperty
//rdfsisDefinedBy rdftype rdfProperty
//rdfvalue rdftype rdfProperty






		{rdfscomment,                     rdfsdomain,        rdfsResource},
		{rdfscomment,                     rdfsrange,         rdfsLiteral},
		{rdfsdomain,                      rdfsdomain,        rdfProperty},
		{rdfsdomain,                      rdfsrange,         rdfsClass},
		{rdffirst,                        rdfsdomain,        rdfList},
		{rdffirst,                        rdfsrange,         rdfsResource},
		{rdfsisDefinedBy,                 rdfsdomain,        rdfsResource},
		{rdfsisDefinedBy,                 rdfsrange,         rdfsResource},
		{rdfslabel,                       rdfsdomain,        rdfsResource},
		{rdfslabel,                       rdfsrange,         rdfsLiteral},
		{rdfsmember,                      rdfsdomain,        rdfsContainer},
		{rdfsmember,                      rdfsrange,         rdfsResource},
		{rdfobject,                       rdfsdomain,        rdfStatement},
		{rdfobject,                       rdfsrange,         rdfsResource},
		{rdfpredicate,                    rdfsdomain,        rdfStatement},
		{rdfpredicate,                    rdfsrange,         rdfProperty},
		{rdfsrange,                       rdfsdomain,        rdfProperty},
		{rdfsrange,                       rdfsrange,         rdfsClass},
		{rdfrest,                         rdfsdomain,        rdfList},
		{rdfrest,                         rdfsrange,         rdfList},
		{rdfsseeAlso,                     rdfsdomain,        rdfsResource},
		{rdfsseeAlso,                     rdfsrange,         rdfsResource},
		{rdfssubClassOf,                  rdfsdomain,        rdfsClass},
		{rdfssubClassOf,                  rdfsrange,         rdfsClass},
		{rdfssubPropertyOf,               rdfsdomain,        rdfProperty},
		{rdfssubPropertyOf,               rdfsrange,         rdfProperty},
		{rdfsubject,                      rdfsdomain,        rdfStatement},
		{rdfsubject,                      rdfsrange,         rdfsResource},
		{rdftype,                         rdfsdomain,        rdfsResource},
		{rdftype,                         rdfsrange,         rdfsClass},
		{rdfvalue,                        rdfsdomain,        rdfsResource},
		{rdfvalue,                        rdfsrange,         rdfsResource}

	});
}
/*
rdfs:subPropertyOf is a partial order:
{} => {?P rdfs:subPropertyOf ?P}                                                              reflexive
{?p1 rdfs:subPropertyOf ?p2. ?p2 rdfs:subPropertyOf ?p3} => {?p1 rdfs:subPropertyOf ?p3}      transitive
{?p1 rdfs:subPropertyOf ?p2. ?p2 rdfs:subPropertyOf ?p1} => {?p1 == ?p2}                      anti-symmetric

*/

//https://www.w3.org/TR/rdf-schema/#ch_subpropertyof
//{?P1 @is rdfs:subPropertyOf ?P2. ?S ?P1 ?O} => {?S ?P2 ?O}.
//
//{?P3 @is rdfs:subPropertyOf rdfs:subPropertyOf. ?P1 ?P3 ?P2} => {?P1 @is rdfs:subPropertyOf ?P2}
//{?P4 @is rdfs:subPropertyOf rdfs:subPropertyOf. ?P3 ?P4 rdfs:subPropertyOf} => {?P3 @is rdfs:subPropertyOf rdfs:subPropertyOf}
//fixed point

//{?P3 @is rdfs:subPropertyOf ?P1. ?S ?P3 ?O}=> {?S ?P1 ?O}
rule_t make_wildcard_rule(nodeid pr)
{
	FUN;
	EEE;
	MSG("..")

	Thing p1 = create_unbound();
	Thing p2 = create_node(pr);
	pred_t sub, p1wildcard, p1lambda;
	nodeid p1p = 0;
	
	//old? //thats just for assert
	//hrm
	//I'm a bit confused about ep-check here. 
	//We ep-out if we repeat a (subject, object, rule) tuple.//subject, pred, object..err..well
	//you could say it both ways
	//Ok so the wildcard rule will be made for each pred
	//We can do that in compile_pred

  //If a pred gets called during its own execution with the same s/o as originally, then we ep out
  //same defined as if we can unify s with orig_s and o with orig_o; if we can then execution of
  //this pred with s and o will be equivalent to execution with orig_s and orig_o, which we're already doing.
  //So, each pred should have its own ep-table, and if execution of a pred ends up calling the same pred,
  //then the 2nd instance should use the same ep-table as the original.

  //That should handle all our ep and wildcard problems.

	ep_t *ep = new_ep();//reminds me, a bug in cppout was that i forgot to pop the ep-pair before the successful unify return and push it back after. Its like..you have a query with two triples calling the same rule...when youre yielding out of the first instance of the rule you want to retract the ep-pair before
//so..this func might need checking wrt that
	DBG(Thing old[2]);
	
	return [entry,ep,DBGC(old) p1,p1p,p2,sub,p1wildcard,p1lambda](Thing *s, Thing *o) mutable {
		setproc("wildcard");
		TRACE_ENTRY;
		
		
		
		return false;
/*
		switch(entry){
		case 0:

			DBG(old[0] = *s);
			// i need to figure out floobits text navigation
			if (find_ep(ep, s, o))
				DONE;
			ep->push_back(thingthingpair(s, o));

			//quite sure its there since its a rdf builtin; should be, at least. it was there last friday.
			sub = ITEM(preds,rdfssubPropertyOf); 
			while (sub(&p1, &p2))
			{
				ASSERT(is_bound(p1));
				{
					Thing *p1v = get_thing(p1);
					ASSERT(is_node(*p1v));
					p1p = get_node(*p1v);
				}

				if (!has(preds, p1p))
					preds[p1p] = make_wildcard_rule(p1p);

				p1lambda = ITEM(preds, p1p);
				while(p1lambda(s, o))//the recursion will happen here
				{
					entry = LAST;
					//todo:retract ep
					return true;
		case_LAST:;
				}
			}
			ASSERT(is_unbound(p1));
			ASSERT(are_equal(old[0], *s));
			ASSERT(ep->size());
			ep->pop_back();
			END;
		}
		*/
	};
}
	




//under construction
void build_in_rules()
{
	build_in_dht();


	/*some commonly used vars*/
	EEE; //char entry = 0;
	coro suc, suc2, ouc;
	Thing *s = nullptr, *o = nullptr;
	Thing ss;
	//Thing oo;
	Thing *r = nullptr;
	
	Thing a,b;
	a = create_unbound();
	b = create_unbound();
	
	/*ep_t *ep = new ep_t();
	eps.push_back(ep)*/

	//pred_t :: function<bool(Thing*,Thing*)>
	pred_t p1, p2;


	/*c is for constant*/
	//Thing c_rdfsType = create_node(op->make(rdftype));
	Thing c_rdfsResource = create_node(rdfsResource);
	Thing c_rdfsClass = create_node(rdfsClass);
	//Thing c_rdfssubClassOf = create_node(op->make(rdfssubClassOf));
	Thing c_rdfProperty = create_node(rdfProperty);




	//rdfs:Resource(?x)
	/*<HMC_a> koo7: you mean if one queries "?x a rdf:Resource" should they get every known subject as a result?
	<HMC_a> the answer would be yes. :-)
	<koo7> HMC_a, every known subject and object, right?
	<koo7> or....every nodeid in the kb thats not in the pred position...where do we draw the line?
	<HMC_a> well, when i say "known subject" i don't really mean everything in the subject position, i mean every node useable as a subject (non-literal)
	<koo7> ok
	<koo7> what do you mean non-literal?
	<HMC_a> you wouldn't bind each int as a result, for example
	<HMC_a> if you returned "0 a Resource" "1 a Resource" "2 a Resource"..... this would be a bit of a problem ;-)
	<koo7> yeah, so everything that explicitly appears in the kb
	<koo7> traverse lists too
	<HMC_a> yes remember that lists are logically just triples as well...
	<koo7> err well wouldnt that mean the bnodes that rdf lists are made up of?
	<HMC_a> so any node name that appears within a list is in the object position of some rdf:first triple
	<HMC_a> yes, the bnode names as well*/
	/*i forgot if we are supposed to/have list triples in compiled preds...if yes this 
	might even produce correct results i guess*/


//so this is saying: if you pass me a constant like a node or a list, then return true, it's a resource, and if you pass me a var, then run through every pred in the kb and give me any node/list that binds to subject or object of any of these preds?yea
//maybe this should instead just iterate thru kb triples tho?
//yea probably; this works but it's basically requiring the evaluation of the entire kb when everything's already defined prior to compile
/*nvm
id rather worry about the semantic difference
anyway we should either consult euler or run with it for now
im fine with running with it; probably the fastest way to get critique from HMC :)
we need tests, hard to make a test when you don't know the should-be
:)
if we roll with it HMC will make tests for us
we should hook up eulersharp at some point though, apparently that's what we're comparing against.. apparently
*/
/*	builtins[rdftype].push_back([a, b, c_rdfsResource, entry, suc, suc2, ouc, s, o, p1](Thing *s_, Thing *o_) mutable {
		map<nodeid, pred_t>::iterator x;
		switch (entry) {
			case 0:
				o = getValue(o_);
				ASSERT(!is_offset(*o));
				ouc = unify(o, &c_rdfsResource);
				while (ouc())
				{
					s = getValue(s_);
					
					
					if(!is_unbound(*s))
					{
						entry = 1;
						return true;
					}
			case 1:
					if(is_unbound(*s))
					{
						for ( x=preds.begin(); x!=preds.end();x++)
						{
							p1 = x->second;//a coro-permanent copy
							while(p1(&a, &b))
							suc = unify(s, &a);
							while(suc())
							{
								entry = 2;
								return true;
			case 2:;
							}
							suc2 = unify(s, &b);
							while(suc2())
							{
								entry = 3;
								return true;
			case 3:;
							}
						}

					}
				}
				END
		}
	});
*/


	
	//todo rdfs:Class(?y) implies (?y(?x) iff rdf:type(?x ?y))
	//...nothing to implement?
	//for this ^, no, but for this v :
	// {?x rdf:type rdfs:Class} => {?x rdfs:subClassOf rdfs:Resource}
	
	{
		pred_t type_pred;
	builtins[rdfssubClassOf].push_back([entry,c_rdfsResource,c_rdfsClass,type_pred,ouc](Thing *s, Thing *res) mutable{
		setproc("type is Class implies superClass is Resource");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			ouc = unify(res, &c_rdfsResource);
			type_pred = ITEM(preds,rdftype);
			entry = LAST;
			while(ouc())
			{
				while(type_pred(s, &c_rdfsClass))
				{
					return true;		
		case_LAST:;
				}
			}
			END;
		}	
	});
	}
	


	// https://www.w3.org/TR/rdf-schema/#ch_domain
	// {?Pred @has rdfs:domain ?Class. ?Instance ?Pred ?Whatever} => {?Instance a ?Class}.
	// rdfs:domain(?x ?y) implies ( ?x(?u ?v)) implies ?y(?u) )
	{
		Thing whatever = create_unbound();
		Thing pred = create_unbound();
		pred_t domain_pred, pred_coro;
		Thing pred_val;
		nodeid pred_nodeid = 0;

		/*this one might still need adding ep check*///really
		//Each pred should have its own ep-table. If execution of this pred ends up calling the same pred again,
		//the new instance should use the same ep-table.
		//We could have a single ep-table for the whole kb. 
		// * Start with an empty graph.
		// * Each triple in a query would make a graph-node, carrying the structure of the triple.
		// * When a triple in the query gets called, it executes rules sequentially. For each rule,
		//   make the children of the graph-node for the triple be the triples in the body, substituted
		//   with the HEAD_S and HEAD_O that result from unifying the query triple with the head triple.
		// * When you want to ep-check anywhere in the query, you just follow the ancestors of a node.
		


		/* Why are we doing domain for rdftype?
		builtins are just grouped by their pred
		just as kb rules are grouped into pred_t's
		rdftype is the predicate of the head of the rule
		rule has head s rdftype o */

	builtins[rdftype].push_back([entry,domain_pred,pred,pred_val,pred_nodeid,pred_coro,whatever](Thing *instance, Thing *cls) mutable {
		setproc("domainImpliesType");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			domain_pred = ITEM(preds,rdfsdomain);
			while (domain_pred(&pred, cls))
			{
				{
					ASSERT(is_bound(pred));
					Thing *pred_val = get_thing(pred);
					//how do we know it's not another bound var good q.......... i guess it wouldnt if a rule returned it
					//at least i put the assert there:) we can test it
					//yeah hm in the long run we should get the floobits session and tmux on one machine i guess
					//so isnt that a bug
					/*i dunno if we're supposed to allow rules to imply this*/
					//if it's  a semantic restriction it should probably be handled more fully
					//but in the typesystem, not here..at least thats my guess 
					/*anyway good catch
					 * tests/rdf/domainImpliesType-tricky
					*/
					ASSERT(is_node(*pred_val));
					nodeid pred_nodeid = get_node(*pred_val);

					//(So if the pred is not there, )a subproperty of it might still satisfy,
					//but we don't have a pred to run, so make a wildcard rule
					if (!has(preds, pred_nodeid))
						preds[pred_nodeid] = make_wildcard_rule(pred_nodeid);
					//If the pred is there, use that. This will need the wildcard rule to be added to the pred during compile_kb
					pred_coro = ITEM(preds, pred_nodeid);
				}
				
				ASSERT(is_unbound(whatever));
				while(pred_coro(instance, &whatever))
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
				ASSERT(is_unbound(whatever));
			}
			return false;
			END;
		}
	});
	}

//	tests/rdf/domainImpliesType passess


//K so should we package these up into one builtin?
	//hell no!
	// {?Pred @has rdfs:range ?Class. ?Whatever ?Pred ?Instance} => {?Instance a ?Class}.
	// rdfs:range(?x ?y) implies ( ?x(?u ?v)) implies ?y(?v) )
//Alright how does that look
	//like a lot of duplicated code hehe yep lol
/*  {
	Thing whatever = create_unbound();
	Thing pred = create_unbound();

  builtins[rdftype].push_back([entry,pred,p1,p2,whatever](Thing *instance, Thing *cls) mutable {
    setproc("rangeImpliesType");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			range_pred = ITEM(preds,rdfsrange);
			while (range_pred(&pred, cls))
			{
				{
					ASSERT(is_bound(pred));
					Thing *pred_val = get_thing(pred);
					//how do we know it's not another bound var good q.......... i guess it wouldnt if a rule returned it
					//at least i put the assert there:) we can test it
					//yeah hm in the long run we should get the floobits session and tmux on one machine i guess
					//so isnt that a bug
					//if it's  a semantic restriction it should probably be handled more fully
					//but in the typesystem, not here..at least thats my guess

					ASSERT(is_node(*pred_val));
					nodeid pred_nodeid = get_node(*pred_val);

					if (!has(preds, pred_nodeid))
						preds[pred_nodeid] = make_wildcard_rule(pred_nodeid);
					pred_coro = ITEM(preds, pred_nodeid);
				}
				ASSERT(is_unbound(whatever));
				while(pred_coro(&whatever,instance))
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
				ASSERT(is_unbound(whatever));
			}
			return false;
			END;
		}

  });
  }

*/



	//rdfs:subClassOf(?x ?y) implies (forall (?u)(?x(?u) implies ?y(?u))
	//{?sub rdfs:subClassOf ?sup. ?something a ?sub} => {?something a ?sup}.
	{
		Thing sub = create_unbound();
		pred_t subclassof_pred, type_coro;

	builtins[rdftype].push_back([entry,sub,subclassof_pred,type_coro](Thing *something, Thing *sup) mutable {
		setproc("XasupIfXasub");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			subclassof_pred = ITEM(preds,rdfssubClassOf);
			while (subclassof_pred (&sub, sup))
			{
				ASSERT(is_bound(sub));
				type_coro = ITEM(preds, rdftype);
				while(type_coro(something, &sub))
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
			}
			ASSERT(is_unbound(sub));
			END;
		}
	});
	}


	//rdfs:Class(?x) implies ( rdfs:subClassOf(?x ?x) and rdfs:subClassOf(?x rdfs:Resource) )


	//{?x rdf:type rdfs:Class} => {?x rdfs:subClassOf ?x}
	/*
	{
	coro suc;
	pred_t type_pred;
	builtins[rdfssubClassOf].push_back([entry,suc,type_pred](Thing *x1, Thing *x2){
		setproc("subClass is reflexive");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			suc = unify(x1,x2);
			entry = LAST;
			while(suc()){
				type_pred = ITEM(preds,rdftype);
				while(type_pred(x1,c_rdfsClass)){
					return true;
		case LAST:
				}
			}
			return false;
		}
	});
	
	}
	
	*/

	//{?x rdf:type rdfs:Class} => {?x rdfs:subClassOf rdfs:Resource}
	//{?x rdfs:subClassOf ?y. ?y rdfs:subClassOf ?z} => {?x rdfs:subClassOf ?z}.
	/*
	{
	Thing y = create_unbound();
	pred_t sub1, sub2;
	builtins[rdfssubClassOf].push_back([entry,y,sub1,sub2](Thing *x, Thing *z){
		setproc("rdfssubClass transitive");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			sub1 = ITEM(preds,rdfssubClassOf);
			entry = LAST;
			while(sub(x,y)){
				sub2 = ITEM(preds,rdfssubClassOf);
				while(sub(y,z)){
					return true;
		case LAST:
				}
			}
			return false;
		
		}
	
	});
	
	}
	
	*/

	//(rdfs:subClassOf(?x ?y) and rdfs:subClassOf(?y ?x)) implies  "?x == ?y" <-- how to handle?


	//{?x rdf:type rdfs:Datatype} => {?x rdfs:subClassOf rdfs:Literal}
	/*
	{
	coro luc;
	Thing l = create_unbound();
	pred_t type_pred;
	builtins[rdfssubClassOf].push_back([entry,l,type_pred](Thing *x, Thing *lit){
		setproc("XaDatatypeThenXsubLiteral");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			luc = unify(l,c_rdfsLiteral);
			entry = LAST;	
			while(luc()){
				type_pred = ITEM(
				while(type_pred(x,c_rdfsDatatype)){
					return true;
		case LAST:
				}
			}
			return false;
		}
	});
	
	}
	
	*/



	//{?x rdf:type rdf:Property} => {?x rdfs:subPropertyOf ?x}
	/*
	{
	coro suc;
	pred_t type_pred;
	builtins[rdfssubPropertyOf].push_back([entry,suc,type_pred,c_rdfProperty](Thing *x1, Thing *x2){
		setproc("rdfssubPropertyOf is reflexive");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			suc = unify(x1,x2);
			entry = LAST;
			while(suc()){
				type_pred = ITEM(preds,rdftype);
				while(type_pred(x1 ,c_rdfProperty)
					return true;
		case LAST:
				}
			}
			return false;
		};
	});
	
	
	}
	*/
	//{?x rdfs:subPropertyOf ?y. ?y rdfs:subPropertyOf ?z} => {?x rdfs:subPropertyOf ?z}
	/*
	{
	Thing y = create_unbound();
	pred_t sub1, sub2;
	builtins[rdfssubPropertyOf].push_back([entry,y](Thing *x, Thing *z){
		setproc("rdfssubPropertyOf is transitive");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			sub1 = ITEM(preds,rdfssubPropertyOf);
			entry = LAST;
			while(sub1(x,y)){
				sub2 = ITEM(preds,rdfssubPropertyOf);
				while(sub2(y,z)){
					return true;
		case LAST:
				}
			}
			return false;
		}
	});
	
	}
	*/
	
	//{?x rdfs:subPropertyOf ?y. ?y rdfs:subPropertyOf ?x} => {?x == ?y} 


	//{?x rdf:type rdfs:ContainerMembershipProperty} => {?x rdfs:subPropertyOf rdfs:member}
	/*
	{
	coro muc;
	pred_t type_pred;
	builtins[rdfssubPropertyOf].push_back([entry,muc,type_pred](Thing *x, Thing *m){
		setproc("XsubpropMember then Xa....");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			muc = unify(m,rdfsmember);
			entry = LAST;
			while(muc()){
				type_pred = ITEM(preds,rdftype);
				while(type_pred(x,rdfsContainerMembershipProperty)){
					return true;
		case LAST:
				}
			}
			return false;
		}
	});
	
	}
	
	
	*/


	/*
19:18 < HMC_a> stoopkid: in rdf there is a tricky caveat, all possible lists are assumed to exist
19:19 < HMC_a> so there are lists that begin with ?x regardless of the value of ?x or state pf the kb....
19:20 < HMC_a> this may seem strange, but it is indeed how cwm, euler, et al (racer, pwlim, jena, the lot) do work
19:24 < HMC_a> stoopkid: well technically it is to match McCarthy's theory of arrays, but that is just minutia really
19:27 < HMC_a> stoopkid: suffice to say that this is a very old soundness-of-closure detail that survived through and was inherited into rdf
19:28 < HMC_a> it is a tricky caveat conceptually, but actually very easy to implement for us :-)
19:46 < HMC_a> stoopkid: it returns with ?l and ?x left as unbound vars!  It returns "?l rdf:first ?x"... in the "result graph" this is a top level statement, so the vars are considered as existential!  So it returns exactly a statement interpreted as "there exists a list l and a node x such that x is the rdf:first of l" which is ofc tautologicaly true regardless of what the kb state is. ;-)
19:46 < HMC_a> our universal vars like ?foo are also bnodes, remember
19:47 < HMC_a> all vars are bnodes, all bnodes are vars! XD
19:47 < stoopkid> ok so it just returns the existential vars, it doesn't return the infinite solution set, gotcha
19:48 < HMC_a> the ones we write like ?foo are universals in subformulae and existential at top level (we use "quantifier alternation" instead of tricky @forall type crap and skolem rewrites and such... mostly just "because we can and it is easier"... ("we can" because we are ordinal and do not allow nesting of formulae at the term rewrite))
19:48 < HMC_a> so nothing rszeno said was wrong wrt more general reasoner semantics, we are just playing on our constraints there wrt skolem et al
19:49 < HMC_a> and "skipping around" some of the more complex semantic details that would occur if we cared for more general formula interpretation :-)
19:49 < HMC_a> anyway
19:51 < stoopkid> now, i'm still iffy on calling vars universals, it seems to me that they are always existential and that it's the {}=>{} that makes for universal quantification
19:51 < HMC_a> the ones we write like _:foo are existentials, and similarly ones like ?foo at a top level in a syntactic unit are considered existential as a sort of special case, and there are some other "tricky" special cases that "sneak in" like variables that appear in a head but not a body end up as something like "doesn't matter which" quantifier in a sense, but again these are all details of our particular reasoner, not of more general rdf.... HEH
19:51 < HMC_a> 19:56 < stoopkid> now, i'm still iffy on calling vars universals, it seems to me that they are always existential and that it's the {}=>{} that makes for universal quantification
19:51 < HMC_a> right, you're not exactly wrong at all...
19:52 < HMC_a> how you are looking at it is sound for our reasoner
19:52 < HMC_a> but is not sound for rdf reasoning in general! hehe
19:53 < HMC_a> in more general rdf reasoning you might run into statements that are structured arbitrarily, where the corresponding predicate fol expression would have arbitrary implication and quantifier placement
19:53 < HMC_a> we do *not* allow arbitrary implication structures *or* arbitrary quantifier placement!! hehe
19:54 < stoopkid> i see
19:54 < HMC_a> well, more accurately, we allow them syntactically but do not reason over them directly in our semantics! :-)
19:55 < HMC_a> we only take "outer" implications (not nested) in our inference chaining, and we determine quantifiers (universal or existential) by placement relative to these
19:56 < HMC_a> someone could ofc write, in our logic, an interpretation that *does* consider things like nested implications, arbitrary quantifications, and a math:sum that will do factoring of objects to bind subject lists...
19:57 < HMC_a> but we do none of these things in our "core" rewrite semantics. :-)
*/

	//if the subject is bound, and bound to a list just take it's first item.
	// If it is bound to something that is not a list, fail.
	// If it is unbound, do a trivial yield (no new binding first).
	// Why the trivial yield on the unbound variable?
	// Returns an existential var; just says, there does exist something which is
	// rdffirst of this bnode. I guess. The only way you can really query with
	// rdffirst is with a var or bnode. 
	builtins[rdffirst].push_back(
			[entry, ouc, s, o](Thing *s_, Thing *o_) mutable {
				setproc("rdffirst");
				switch (entry) {
					case 0:
						s = getValue(s_);
						o = getValue(o_);
						TRACE(dout << str(s) << " #first " << str(o) << endl);
						if (is_unbound(*s))
							ouc = gen_succeed();
						//Couldn't we just:
						/*
							entry = LAST;
							return true;

						*/

						//Not sure i understand the +1 +2 here
						else if (is_list(*s))
							ouc = unify(o, s + 2);
						else if (is_list_bnode(*s))
							ouc = unify(o, s + 1);
						else
							ouc = GEN_FAIL;
						entry = LAST;
						while (ouc())
						{
							return true;
							case_LAST:;
						}
						END
				}
			}
	);

	builtins[rdfrest].push_back(
			[entry, ouc, s, o, r](Thing *s_, Thing *o_) mutable {
				setproc("rdfrest");
				switch (entry) {
					case 0:
						s = getValue(s_);
						o = getValue(o_);
						TRACE(dout << str(s) << " #rest " << str(o) << endl);
						//not sure i understand the +3 +2 here
						if (is_list(*s))
							r = s+3;
						else if(is_list_bnode(*s))
							r = s+2;
						else {
	
							entry = 66;
							return false;
						}

						ouc = unify(o, r);
						entry = LAST;
						while (ouc())
						{
							return true;
							case_LAST:;
						}
						END
				}
			}
	);








/*
<HMC_a> koo7: for the moment I'm less concerned about getting rdfs going and more interested in facilities like log:outputString and math:sum and etc
<HMC_a> really even just those two would be enough to get some useful results out of the fuzzer, lol :-)
<HMC_a> http://www.w3.org/2000/10/swap/doc/CwmBuiltins <-- "not quite specs" XD
<HMC_a> so math:* and string:* should be pretty straightforward, list:* nearly so...
<koo7> as on list, list:append seems a misnomer
<HMC_a> koo7: you mean it is more aptly called "concat"? ;-)
<koo7> yeah something like that
<HMC_a> I don't think you're the first to mention it, hehe
* HMC_a shrugs
<koo7> alright
<koo7> its fully namespaced, after all
<HMC_a> sure, and I'm not against doubling up on some builtins later... maybe in the end we have list:append, tau:append, and tau:concat, with tau:append taking just a subject of a pair list of a list and a literal and doing an actual "append" and the other two both being "concat"...
<HMC_a> but we want list:append to be there and to match cwm, in any case, so that any existing logic "out there" that calls upon cwm's list:append will do the right thing
 */



	/*
	 * @prefix math: <http://www.w3.org/2000/10/swap/math#>.
	 */



	//sum: The subject is a list of numbers. The object is calculated as the arithmentic sum of those numbers.

	//why not call it pred or pred_iri
	string bu = "http://www.w3.org/2000/10/swap/math#sum";
	auto bui = dict.set(mkiri(pstr(bu)));

	builtins[bui].push_back(
			[r, bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				//TRACE_ENTRY?
				switch (entry) {
					case 0: {

						Thing *sv = getValue(s_);
						//At this point s should not be either a bound
						//var or an offset.

						
				//At this point neither s nor ss should be either bound vars or offsets.


				//Is this an error? //we just dont unify
						if (!is_listey(sv)) {
							dout << bu << ": " << str(s_) << " not a list" << endl;
							DONE;
						}
						

						long total = 0;
						Thing *item;
						while((item = next_item(sv)))
						{
							//i think we should be getValuing here
							item = getValue(item);
							
				//Make sure it's a node. Is this an error if it's not?
							if (!is_node(*item)) {
								dout << bu << ": " << str(item) << " not a node" << endl;
								DONE;
							}
				
							node p = dict[get_node(*item)];
				//Make sure it's an XSD_INTEGER. Is this an error if it's not?
							if (p.datatype != XSD_INTEGER) {
								dout << bu << ": " << p.tostring() << " not an int" << endl;
								DONE;
							}

				//Convert the text value to a long, and add
				//it to the total.
							total += atol(ws(*p.value).c_str());
						}

				//Convert the total sum to a text value.
						std::stringstream ss;
						ss << total;

				//Create an XSD_INTEGER node & corresponding Thing for the total
						r = new(Thing);
						*r = create_node(dict[mkliteral(pstr(ss.str()), pstr("XSD_INTEGER"), 0)]);

				//Unify the input object with the Thing now representing the total.
						ouc = unify(o_, r);
					}
						while (ouc()) {
							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;
					case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
				//No longer need the Thing representing the total. hmm
				//How about the node representing the total?
						delete (r);
						END;
				}
			}
	);


	/*
	 * @prefix log: <http://www.w3.org/2000/10/swap/log#>.
	 * */

/*
//outputString	The subject is a key and the object is a string, where the strings are to be output in the order of the keys. See cwm --strings in cwm --help.
<koo7> Dump :s to stdout ordered by :k whereever { :k log:outputString :s }
<koo7> so this means it waits until the query is done?
<HMC_a> yes, it caches up the output strings until the end
<HMC_a> then sorts them by subject
<HMC_a> then prints
 */
	bu = "http://www.w3.org/2000/10/swap/log#outputString";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_node(s2)) {
							dout << bu << ": " << str(s) << " not a node" << endl;
							DONE;
						}
						auto o = getValue(o_);
						Thing o2 = *o;
						if (!is_node(o2)) {
							dout << bu << ": " << str(o) << " not a node" << endl;
							DONE;
						}
						auto sss = dict[get_node(s2)].tostring();
						auto ooo = dict[get_node(o2)].tostring();

						log_outputString[sss] = ooo;
						entry = LAST;
						return true;
					}
					case_LAST:;
						END;
				}
			}
	);
#include "printnow.cpp"

}









//https://www.w3.org/TR/rdf-schema/#ch_type
//http://wifo5-03.informatik.uni-mannheim.de/bizer/SWTSGuide/carroll-ISWC2004.pdf

//Unicode

//XML Schema
//http://www.w3.org/TR/2009/CR-xmlschema11-2-20090430/

//LBase
//http://www.w3.org/TR/lbase/

//RDF
//http://www.w3.org/2011/rdf-wg/wiki/Main_Page
//http://www.w3.org/TR/2014/NOTE-rdf11-primer-20140225/
//http://www.w3.org/TR/2013/WD-rdf11-mt-20130409/
//http://www.w3.org/TR/rdf11-new/
//http://www.w3.org/TR/rdf11-concepts/
//http://www.w3.org/TR/rdf-syntax-grammar/
//http://www.w3.org/TR/2014/NOTE-rdf11-datasets-20140225/

//N-Quads
//http://www.w3.org/TR/2014/REC-n-quads-20140225/

//N-Triples
//http://www.w3.org/TR/n-triples/

//JSON

//JSON-LD
//http://www.w3.org/TR/json-ld/

//Notation 3
//

//RIF
//http://www.w3.org/standards/techs/rif#w3c_all
//http://www.w3.org/TR/rif-dtb/
//http://www.w3.org/TR/2013/REC-rif-dtb-20130205/
//http://sourceforge.net/p/eulersharp/discussion/263032/thread/d80e9aa6/

//Cwm Builtins
//http://www.w3.org/2000/10/swap/doc/CwmBuiltins   	--< HMC_a_> not all but most
//which?

//DTLC
//http://rbjones.com/rbjpub/logic/cl/tlc001.htm
//http://ceur-ws.org/Vol-354/p63.pdf

//OWL
//http://www.w3.org/TR/owl2-overview/

//make our semantics conform to them! ^














/*log:equalTo a rdf:Property;
True if the subject and object are the same RDF node (symbol or literal).
Do not confuse with owl:sameAs.
A cwm built-in logical operator, RDF graph level.
*/


	///std::sort(myList.begin(), myList.end(), [](int x, int y){ return std::abs(x) < std::abs(y); });
	///sort(facts.begin(), facts.end(), [](auto a, auto b) { return a[1] < b[1]; });





	/*
	//@prefix list: <http://www.w3.org/2000/10/swap/list#>.
	//list last item
	bu = "http://www.w3.org/2000/10/swap/list#last";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_list(s2)) {
							dout << bu << ": " << str(s) << " not a list" << endl;
							DONE;
						}

						auto size = get_size(s2);
						if (size == 0) DONE;
						ouc = unify(s + size, o_);
					}
						while (ouc()) {
							entry = LAST;
							return true;
							case_LAST:;
						}
						END;
				}
			}
	);
	 */

	/*
	//nope
	//item in list
	bu = "http://www.w3.org/2000/10/swap/list#in";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_node(s2)) {
							dout << bu << ": " << str(s) << " not a node" << endl;
							DONE;
						}
						auto t = get_term(s2);

						auto o = getValue(o_);
						Thing o2 = *o;
						if (!is_list(o2)) {
							dout << bu << ": " << str(o) << " not a list" << endl;
							DONE;
						}

						const auto size = get_size(o2);
						//?bool found = false;

						for (size_t i = 0; i < size; i++) {
							Thing item = *(o + 1 + i);
							if (is_node(item)) {
								if (t == get_term(item)) {
									entry = LAST;
									return true;
								}
							}
						}
					}
					case_LAST:;
						END;
				}
			}
	);
	 */

