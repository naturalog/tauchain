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
#include "prover.h"
boost::interprocess::list<thread*> threads;

shared_ptr<predicate> reasoner::mkpred ( string s, const predlist& v ) {
	return make_shared<predicate>( dict.has ( s ) ? dict[s] : dict.set ( s ), v );
}

shared_ptr<const predicate> reasoner::triple ( const string& s, const string& p, const string& o ) {
	return mkpred ( p, { mkpred ( s ), mkpred ( o ) } );
}

shared_ptr<const predicate> reasoner::triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
}

prover::term* pred2term(shared_ptr<const predicate> p) {
	if (!p) return 0;
	prover::term* t = &prover::terms[prover::nterms++];
	t->p = dict.set(dstr(p->pred));
	if (p->args.size()) {
		t->s = pred2term(p->args[0]);
		t->o = pred2term(p->args[1]);
	} else
		t->s = t->o = 0;
/*	TRACE(dout<<*p<<endl);
	TRACE(printterm(*t));
	TRACE(dout<<p->pred<<endl);
	TRACE(dout<<endl<<t->p<<endl);
	TRACE(dout<<t->p<<endl);
	TRACE(dout<<endl);*/
	TRACE(dout<<t->p<<endl);
	return t;
}

int szqdb(const qdb& x) {
	int r = 0;
	for (auto y : x)
		r += y.second->size();
	return r;
}

void reasoner::addrules(string s, string p, string o, prover::session& ss, const qdb& kb) {
	if (p[0] == '?')
		throw 0;
	prover::rule* r = &prover::rules[prover::nrules++];
	r->p = pred2term(triple ( s, p, o ));
	if ( p != implication || kb.find ( o ) == kb.end() ) 
		ss.rkb[r->p->p].push_back(r);
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		r = &prover::rules[prover::nrules++];
		prover::term* tt = pred2term(triple ( *y ));
		TRACE(dout<<tt->p<<endl);
		r->p = tt;
		TRACE(printr(*r));
		TRACE(dout<<endl);
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				r->body.push_front( pred2term( triple ( *z ) ) );
		ss.rkb[r->p->p].push_back(r);
		TRACE(printr(*r));
		TRACE(dout<<endl);
	}
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

bool haspredvar(const qdb& x) {
	return false;
	for (auto y : x)
		for (auto quad : *y.second)
			if (quad->pred->value[0] == '?') {
				dout << quad->tostring() << endl;
				return true;
			}
	return false;
}

bool reasoner::prove ( qdb kb, qlist query ) {
	prover::session ss;
	set<string> predicates;
	for ( jsonld::pquad quad : *kb.at("@default")) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
//		if (p[0] == '?' || (p.find('#') != string::npos && s[p.find('#')+1] == '?'))
//			for (string pr : predicates)
//				addrules(s, pr, o, ss, kb);
//		else
			addrules(s, p, o, ss, kb);
	}
	for ( auto q : query )
		ss.goal.push_front( pred2term( triple ( *q ) ) );
	prover::prove(&ss);
	return ss.e.size();
}

bool reasoner::test_reasoner() {
/*

to perform the socrates test, put the following three lines in a file say f, and run "tau < f":
socrates a man.  ?x a man _:b0.  ?x a mortal _:b1.  _:b0 => _:b1.  
fin.  
?p a mortal.

*/
	return 1;
}

float degrees ( float f ) {
	static float pi = acos ( -1 );
	return f * 180 / pi;
}

/*
int proof::builtin ( const predicate* t_ ) {
	if (t_ && dict[t_->pred] == "GND") return 1;
	
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
	else if ( p == "a" && t1 && dict[t1->pred] == "rdfs:Resource" ) return 1;
	return -1;
}*/
