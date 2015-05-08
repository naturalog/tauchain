/*
    euler.cpp

    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include "proof.h"
#include "parsers.h"
#include "rdf.h"
#include <thread>
#include "misc.h"
#include <assert.h>

predicate *temp_preds = new predicate[max_predicates];
boost::interprocess::vector<proof*> proofs;
uint ntemppreds = 0;

reasoner::reasoner() : GND ( &predicates[npredicates++].init ( dict.set ( "GND" ), {}, {} ) ) {}

predicate *predicates = new predicate[max_predicates];
//proof *proofs = new proof[max_proofs];
uint npredicates = 0, nproofs = 0;

reasoner::~reasoner() {
	delete[] predicates;
//	delete[] proofs;
}

predicate& predicate::init ( int _p, predlist _args, predlist _body ) {
	pred = _p;
	args = _args;
	body = _body;
	return *this;
}

proof& proof::init ( const proof& f ) {
	return init ( f.rul, f.ind, f.parent, f.sub, f.ground );
}

proof& proof::init ( const predicate* _r, uint _ind, const proof* p, subst _s, ground_t _g ) {
	if ( nproofs >= max_proofs ) throw "Buffer overflow";
	proof& f = *new proof;//proofs[nproofs++];
	proofs.push_back(&f);
	nproofs++;
	f.rul = _r;
	f.ind = _ind;
	f.parent = p;
	f.sub = _s;
	f.ground = _g;
	return f;
}

void reasoner::printkb() {
	static bool pause = false;
	cout << endl << "dumping kb with " << npredicates << " predicates, " << npredicates << " predicates and " << nproofs << " proofs. " << endl;
	cout << "predicates: " <<  endl;
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	cout << "proofs: " << endl;
	for ( uint n = 0; n < nproofs; ++n ) cout << proofs[n] << endl;
	if ( pause ) cout << "type <enter> to continue or <c><enter> to stop pausing...";
	cout << endl;
	if ( pause && getchar() == 'c' ) pause = false;
}

const predicate* predicate::evaluate ( const subst& sub ) const {
	trace ( "\tEval " << *this << " in " << sub << endl );
	if ( pred < 0 ) {
		auto it = sub.find ( pred );
		return it == sub.end() ? 0 : it->second->evaluate ( sub );
	} else { // no braces here in euler.js
		if ( args.empty() ) return this;
	}
	const predicate *p;
	predicate* r = &temp_preds[ntemppreds++].init ( pred, {}, {} );
	for ( auto x : args )
		r->args.emplace_back ( ( p = x->evaluate ( sub ) ) ? p : &temp_preds[ntemppreds++].init ( x->pred, {}, {} ) );
	return r;
}

const predicate* unify ( const predicate& s, const subst& ssub, const predicate& d, subst& dsub, bool f ) {
	trace ( "\tUnify s: " << s << " in " << ( ssub ) << " with " << d << " in " << dsub << endl );
	const predicate* p;
	if ( s.pred < 0 ) {
		if ( ( p = s.evaluate ( ssub ) ) ) return unify ( *p, ssub, d, dsub, f );
		else {
			trace ( "Match." << endl );
			return &s;
		}
	}
	if ( d.pred >= 0 ) {
		if ( s.pred != d.pred || s.args.size() != d.args.size() ) return 0;
		const predlist& as = s.args, ad = d.args;
		for ( auto sit = as.begin(), dit = ad.begin(); sit != as.end(); ++sit, ++dit )
			if ( !unify ( **sit, ssub, **dit, dsub, f ) )
				return 0;
		trace ( "Match." << endl );
		return &s;
	}
	if ( ( p = d.evaluate ( dsub ) ) ) return unify ( s, ssub, *p, dsub, f );
	if ( f ) dsub[d.pred] = s.evaluate ( ssub );
	trace ( "Match with subst: " << dsub << endl );
	return &d;
}

proof& proof::find ( const predicate* goal, const cases_t& cases) {
	proof& f = init( goal );
	evidence_t& e = *new evidence_t;
	f.process( *new cases_t(cases), e );
	return f;
}
boost::interprocess::list<thread*> threads;

void proof::process ( const cases_t& cases, evidence_t& evidence ) {
	auto f = [this, &cases, &evidence](){
	trace ( *this << endl );
	bool empty = true;
	if ( ind >= rul->body.size() ) {
		if ( !parent )
			for ( const predicate* x : rul->body ) {
				const predicate* t = x->evaluate ( sub );
				evidence[t->pred].emplace_back ( t, ground );
			}
		else {
			proof& new_proof = init ( parent->rul, parent->ind + 1, parent->parent, parent->sub, ground );
			if ( !rul->body.empty() ) new_proof.ground.emplace_front ( rul, sub );
			if ( rul->pred ) unify ( *rul, sub, *new_proof.rul->body[new_proof.ind - 1], new_proof.sub, true );
			new_proof.process(cases, evidence);
			empty = false;
		}
	} else {
		const predicate* t = rul->body[ind];
		trace("fetched predicate " << *t <<endl);
		int b = builtin ( t );
		if ( b == 1 ) {
			proof& r = proof::init ( *this );
			r.ground = ground;
			r.ground.emplace_back ( t->evaluate ( sub ), subst() );
			r.ind++;
			//next.push_back ( &r );
			r.process(cases, evidence);
			empty = false;
		} else if ( !b ) return;
		auto it = cases.find ( t->pred );
		if ( it != cases.end() )
			for ( const predicate* rl : it->second ) {
				subst s;
				if ( /*!rl->head || */!unify ( *t, sub, *rl, s, true ) ) continue;
				const proof* ep = parent;
				for (; ep; ep = ep->parent)
					if ( ( ep->rul == rul ) && unify ( *ep->rul, ep->sub, *rul, const_cast<subst&>(sub), false ) )
						break;
				if ( ep && ep->parent ) continue;
				ground_t g;
				if (rl->body.empty()) g.emplace_back( rl, subst() );
				//next.push_front( 
				proof::init ( rl, 0, this, s, g ).process(cases, evidence);
				empty = false;
			}
		}
	if (empty) cout << "evidence: " << evidence << endl;
	};
//	threads.push_back(new thread(f));
	f();
}

predicate* reasoner::mkpred ( string s, const predlist& v ) {
	int p = dict.has ( s ) ? dict[s] : dict.set ( s );
	for (uint n = 0; n < npredicates; ++n) if (predicates[n].pred == p && predicates[n].args == v) return &predicates[n];
	return &predicates[npredicates++].init ( p, v, {} );
}

predicate* reasoner::triple ( const string& s, const string& p, const string& o ) {
	return mkpred ( p, { mkpred ( s ), mkpred ( o ) } );
}

predicate* reasoner::triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

evidence_t reasoner::prove ( const qdb &kb, const qlist& query ) {
	evidence_t evidence;
	cases_t cases;
	trace ( "Reasoner called with quads kb: " << endl << kb << endl << "And query: " << endl << query << endl );
	for ( const pair<string, jsonld::pqlist>& x : kb ) {
		for ( jsonld::pquad quad : *x.second ) {
			const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
			trace ( "processing quad " << quad->tostring() << endl );
			cases[dict[p]].push_back ( triple ( s, p, o ) );
			if ( p != implication || kb.find ( o ) == kb.end() ) continue;
			for ( jsonld::pquad y : *kb.at ( o ) ) {
				predicate& rul = *triple(*y);
//				rul.head = triple ( *y );
				if ( kb.find ( s ) != kb.end() )
					for ( jsonld::pquad z : *kb.at ( s ) )
						rul.body.push_back ( triple ( *z ) );
				cases[rul.pred].push_back ( &rul );
				trace ( "added predicate " << rul << endl );
			}
		}
	}
	predicate& goal = predicates[npredicates++].init(0, {}, {});
	for ( auto q : query ) goal.body.push_back ( triple ( *q ) );
//	printkb();
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	return prove ( &goal, -1, cases );
}

bool reasoner::test_reasoner() {
	dict.set ( "a" );
	//	cout <<"dict:"<<endl<< dict.tostr() << endl;
	//	exit(0);
	evidence_t evidence;
	cases_t cases;
//	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );

	qdb q, qu;
	pqlist ql = make_shared<qlist>();
	string g = "@default";
	ql->push_back(make_shared<quad>("Socrates", "a", "Male", g));
	ql->push_back(make_shared<quad>("_:b1", implication, "_:b0", g));
	ql->push_back(make_shared<quad>("_:b2", implication, "_:b1", g));
	q[g] = ql;
	ql = make_shared<qlist>();
	ql->push_back(make_shared<quad>("?x", "a", "Mortal", "_:b0"));
	q["_:b0"] = ql;
	ql = make_shared<qlist>();
	ql->push_back(make_shared<quad>("?x", "a", "Man", "_:b1"));
	q["_:b1"] = ql;
	ql = make_shared<qlist>();
	ql->push_back(make_shared<quad>("?x", "a", "Male", "_:b2"));
	q["_:b2"] = ql;
	ql = make_shared<qlist>();
	ql->push_back(make_shared<quad>("?y", "a", "Mortal", g));
	qu[g] = ql;
	cout<<q<<endl;
	cout<<qu<<endl;
	prove(q, *ql);
/*
	cases[dict["a"]].push_back ( triple( "Socrates", "a", "Male" ) );
	{
	predicate& pr = *triple( "?x", "a", "Mortal");
	pr.body.push_back(triple( "?x", "a", "Man"));
	cases[pr.pred].push_back( &pr );
	}
	{
	predicate& pr = *triple( "?x", "a", "Man");
	pr.body.push_back(triple( "?x", "a", "Male"));
	cases[pr.pred].push_back( &pr );
	}
*/
//	cases[dict["a"]].push_back ( &predicates[npredicates++].init(0, {},{mkpred ( "a", {Socrates, Male} )}) );
//	cases[dict["a"]].push_back ( &predicates[npredicates++].init(0, {},{mkpred ( "a", {_x, Mortal} ), predlist{ mkpred ( "a", {_x, Man } )  }}) );
//	cases[dict["a"]].push_back ( &predicates[npredicates++].init(0, {},{mkpred ( "a", {_x, Man   } ), predlist{ mkpred ( "a", {_x, Male} )  }}) );

//	predicate* goal = triple("?y", "a", "Mortal");// mkpred ( "a", { _y, Mortal } );
//	evidence = prove ( goal, -1, cases );
//	cout << "evidence: " << evidence.size() << " items..." << endl;
//	cout << evidence << endl;
//	cout << "QED!" << endl;
//	cout << evidence.size() << endl;
	return evidence.size();
}

float degrees ( float f ) {
	static float pi = acos ( -1 );
	return f * 180 / pi;
}


int proof::builtin ( const predicate* t_ ) {
	if (t_ && dict[t_->pred] == "GND") return 1;
/*	
	if ( !t_ ) return -1;
	const predicate& t = *t_;
	string p = dict[t.pred];
	if ( p == "GND" ) return 1;
	if ( t.args.size() != 2 ) throw runtime_error ( "builtin expects to get a predicate with spo" );
	const predicate* t0 = evaluate ( **t.args.begin(), f.sub );
	const predicate* t1 = evaluate ( **++t.args.begin(), f.sub );
	if ( p == "log:equalTo" ) return ( ( t0 && t1 && t0->pred == t1->pred ) ? 1 : 0 );
	else if ( p == "log:notEqualTo" ) return ( t0 && t1 && t0->pred != t1->pred ) ? 1 : 0;
	else if ( p == "math:absoluteValue" ) {
		if ( t0 && t0->pred > 0 )
			return ( unify ( mkpred ( tostr ( fabs ( stof ( dict[t0->pred] ) ) ) ), f.sub, t.args[1], f.sub, true ) ) ? 1 : 0;
	}
#define FLOAT_BINARY_BUILTIN(x, y) \
  else if (p == x) \
      return \
        (t0 && t0->pred > 0 && \
      	(unify(mkpred(tostr(y(stof(dict[t0->pred])))), f.sub, t.args[1], f.sub, true))) || \
        (t1 && t1->pred > 0 && \
      	(unify(mkpred(tostr(y(stof(dict[t0->pred])))), f.sub, t.args[0], f.sub, true))) \
	? 1 : 0
	FLOAT_BINARY_BUILTIN ( "math:degrees", degrees );
	FLOAT_BINARY_BUILTIN ( "math:cos", cos );
	FLOAT_BINARY_BUILTIN ( "math:sin", cos );
	FLOAT_BINARY_BUILTIN ( "math:tan", cos );
	else if ( p == "math:equalTo" ) return ( t0 && t1 && stof ( dict[t0->pred] ) == stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:greaterThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) > stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:lessThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) < stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:notEqualTo" ) return ( t0 && t1 && stof ( dict[t0->pred] ) != stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:notLessThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) >= stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:notGreaterThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) <= stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:equalTo" ) return ( t0 && t1 && stof ( dict[t0->pred] ) == stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:difference" && t0 ) {
		float a = stof ( dict[ ( evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred )] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a -= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] );
		return ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) ? 1 : 0;
	} else if ( p == "math:exponentiation" && t0 ) {
		float a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred] );
		for ( auto ti = t0->args[1]; ti && !ti->args.empty(); ti = ti->args[1] ) a = pow ( a, stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] ) );
		return ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) ? 1 : 0;
	} else if ( p == "math:integerQuotient" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a /= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] );
		return ( unify ( mkpred ( tostr ( floor ( a ) ) ), f.sub, t.args[1], f.sub, true ) ) ? 1 : 0;
	} else if ( p == "math:negation" ) {
		if ( t0 && t0->pred >= 0 ) {
			auto a = -stof ( dict[t0->pred] );
			if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) return 1;
		} else if ( t1 && t1->pred >= 0 ) {
			auto a = -stof ( dict[t1->pred] );
			if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[0], f.sub, true ) ) return 1;
		} else return 0;
	} else if ( p == "math:product" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a *= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) return 1;
		else return 0;
	} else if ( p == "math:quotient" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a /= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) return 1;
		else return 0;
	} else if ( p == "math:remainder" && t0 ) {
		auto a = stoi ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a %= stoi ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) return 1;
		else return 0;
	} else if ( p == "math:rounded" ) {
		if ( t0 && t0->pred >= 0 ) {
			float a = round ( stof ( dict[t0->pred] ) );
			if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) return 1;
		} else return 0;
	} else if ( p == "math:sum" && t0 ) {
		float a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.sub )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a += stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.sub )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.sub, t.args[1], f.sub, true ) ) return 1;
		else return 0;
	} else if ( p == "rdf:first" && t0 && dict[t0->pred] == "." && !t0->args.empty() )
		return ( unify ( t0->args[0], f.sub, t.args[1], f.sub, true ) ) ? 1 : 0;
	else if ( p == "rdf:rest" && t0 && dict[t0->pred] == "." && !t0->args.empty() )
		return ( unify ( t0->args[1], f.sub, t.args[1], f.sub, true ) ) ? 1 : 0;
	else if ( p == "a" && t1 && dict[t1->pred] == "rdf:List" && t0 && dict[t0->pred] == "." ) return 1;
	else if ( p == "a" && t1 && dict[t1->pred] == "rdfs:Resource" ) return 1;*/
	return -1;
}

string predicate::dot() const {
	stringstream r;
	for (auto x : body) {
		r << '\"' << *x << '\"';
		r << " -> " << '\"' << *this << '\"';
	//	else r << " [shape=box]";
		r << ';' << endl;
	}
	return r.str();
}
