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

predicate *temp_preds = new predicate[max_predicates];
boost::interprocess::vector<proof*> proofs;
uint ntemppreds = 0;

reasoner::reasoner() : GND ( &predicates[npredicates++].init ( dict.set ( "GND" ) ) ) {}

predicate *predicates = new predicate[max_predicates];
rule *rules = new rule[max_rules];
//proof *proofs = new proof[max_proofs];
uint npredicates = 0, nrules = 0, nproofs = 0;

reasoner::~reasoner() {
	delete[] predicates;
	delete[] rules;
//	delete[] proofs;
}

rule& rule::init ( const predicate* h, predlist b ) {
	head = h;
	body = b;
	return *this;
}

proof& proof::init ( const proof& f ) {
	return init ( f.rul, f.ind, f.parent, f.sub, f.ground );
}

proof& proof::init ( const rule* _r, uint _ind, const proof* p, subst _s, ground_t _g ) {
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
	cout << endl << "dumping kb with " << npredicates << " predicates, " << nrules << " rules and " << nproofs << " proofs. " << endl;
	cout << "predicates: " <<  endl;
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	cout << "rules: " << endl;
	for ( uint n = 0; n < nrules; ++n ) cout << rules[n] << endl;
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
	predicate* r = &temp_preds[ntemppreds++].init ( pred );
	for ( auto x : args )
		r->args.emplace_back ( ( p = x->evaluate ( sub ) ) ? p : &temp_preds[ntemppreds++].init ( x->pred ) );
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

proof& proof::find ( const rule* goal, const cases_t& cases) {
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
			if ( rul->head ) unify ( *rul->head, sub, *new_proof.rul->body[new_proof.ind - 1], new_proof.sub, true );
			new_proof.process(cases, evidence);
			empty = false;
		}
	} else {
		const predicate* t = rul->body[ind];
		int b = builtin ( t );
		if ( b == 1 ) {
			proof& r = proof::init ( *this );
			r.ground = ground;
			r.ground.emplace_back ( &rules[nrules++].init ( t->evaluate ( sub ) ), subst() );
			r.ind++;
			//next.push_back ( &r );
			r.process(cases, evidence);
			empty = false;
		} else if ( !b ) return;
		auto it = cases.find ( t->pred );
		if ( it != cases.end() )
			for ( const rule* rl : it->second ) {
				subst s;
				if ( !rl->head || !unify ( *t, sub, *rl->head, s, true ) ) continue;
				const proof* ep = parent;
				for (; ep; ep = ep->parent)
					if ( ( ep->rul == rul ) && unify ( *ep->rul->head, ep->sub, *rul->head, const_cast<subst&>(sub), false ) )
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

rule* reasoner::mkrule ( const predicate* p, const predlist& v ) {
	return &rules[nrules++].init ( p, v );
}

predicate* reasoner::mkpred ( string s, const predlist& v ) {
	return &predicates[npredicates++].init ( dict.has ( s ) ? dict[s] : dict.set ( s ), v );
}

const predicate* reasoner::triple ( const string& s, const string& p, const string& o ) {
	return mkpred ( p, { mkpred ( s ), mkpred ( o ) } );
}

const predicate* reasoner::triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}
/*
orig from f429a7d:
evidence_t reasoner::prove ( const qdb &kb, const qlist& query ) {
	evidence_t evidence;
	cases_t cases;
	trace ( "Reasoner called with quads kb: " << endl << kb << endl << "And query: " << endl << query << endl );
	for ( const pair<string, jsonld::pqlist>& x : kb ) {
		for ( jsonld::pquad quad : *x.second ) {
			const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
			trace ( "processing quad " << quad->tostring() << endl );
			cases[dict[p]].push_back ( mkrule ( triple ( s, p, o ) ) );
			if ( p != implication || kb.find ( o ) == kb.end() ) continue;
			for ( jsonld::pquad y : *kb.at ( o ) ) {
				rule& rul = *mkrule();
				rul.head = triple ( *y );
				if ( kb.find ( s ) != kb.end() )
					for ( jsonld::pquad z : *kb.at ( s ) )
						rul.body.push_back ( triple ( *z ) );
				cases[rul.head->pred].push_back ( &rul );
				trace ( "added rule " << rul << endl );
			}
		}
	}
	rule& goal = *mkrule();
	for ( auto q : query ) goal.body.push_back ( triple ( *q ) );
//	printkb();
	return prove ( &goal, -1, cases );
}
*/

prover::term* pred2term(const predicate* p, prover::dict** d) {
	if (!p) return 0;
	prover::term* t = &prover::terms[prover::nterms++];
	t->p = prover::pushw(d, dstr(p->pred).c_str());
	if (p->args.size()) {
		t->s = pred2term(p->args[0], d);
		t->o = pred2term(p->args[1], d);
	} else
		t->s = t->o = 0;
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
	r->p = pred2term(triple ( s, p, o ), &ss.d);
	r->body = 0;
	if ( p != implication || kb.find ( o ) == kb.end() ) 
		prover::pushr(&ss.rkb, r);
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		r = &prover::rules[prover::nrules++];
		r->p = pred2term(triple ( *y ), &ss.d);
		r->body = 0;
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				prover::pushp(&r->body, pred2term( triple ( *z ), &ss.d ) );
		prover::pushr(&ss.rkb, r);
//				trace ( "added rule " << rul << endl );
	}
}

bool haspredvar(const qdb& x) {
	return false;
	for (auto y : x)
		for (auto quad : *y.second)
			if (quad->pred->value[0] == '?') {
				cout << quad->tostring() << endl;
				return true;
			}
	return false;
}

bool reasoner::prove ( qdb kb, qlist query ) {
	prover::session ss;
	memset(&ss, 0, sizeof(prover::session));
	set<string> predicates;
/*	qdb tmp;
	for (auto x : kb)
		for (auto q : *x.second)
			if (q->pred->value[0] != '?') 
				for (auto _z : kb) 
					for (auto quad : *_z.second) {
						string s = quad->subj->value, p = quad->pred->value, o = quad->object->value;
						if (p[0] != '?') {
							if (tmp.find(x.first) == tmp.end())
								tmp[x.first] = make_shared<jsonld::qlist>();
							tmp[x.first]->push_back(make_shared<jsonld::quad>(s, p, o, _z.first));
						} else for (auto t : kb) {
							for (auto t1 : *t.second) {
								p = t1->pred->value;
								if (p[0] != '?') {
									if (tmp.find(t.first) == tmp.end())
										tmp[t.first] = make_shared<jsonld::qlist>();
									tmp[t.first]->push_back(make_shared<jsonld::quad>(s, p, o, t.first));
								}
							}
						}
					}
//				predicates.insert(quad->pred->value);
	kb = tmp;
	while (haspredvar(kb)) {
		cout << szqdb(kb) << endl;
		for (auto x : kb)
			for (auto q : *x.second)
				if (q->pred->value[0] != '?') {
					x.second->remove(q);
					break;
				}
	}
	for (auto x : tmp)
		kb[x.first] = x.second;*/
	for ( jsonld::pquad quad : *kb.at("@default")) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		cout << "PRED: " << p << endl;
		if (p[0] == '?' || (p.find('#') != string::npos && s[p.find('#')+1] == '?'))
			for (string pr : predicates)
				addrules(s, pr, o, ss, kb);
		else
			addrules(s, p, o, ss, kb);
	}
	rule& goal = *mkrule();
	for ( auto q : query ) 
		prover::pushp(&ss.goal, pred2term( triple ( *q ), &ss.d ) );
//	printkb();
	prover::prove(&ss);
	return ss.e;
}

bool reasoner::test_reasoner() {
	dict.set ( "a" );
	//	cout <<"dict:"<<endl<< dict.tostr() << endl;
	//	exit(0);
//	evidence_t evidence;
	cases_t cases;
	typedef predicate* ppredicate;
	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {Socrates, Male} ) ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Mortal} ), predlist{ mkpred ( "a", {_x, Man } )  } ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Man   } ), predlist{ mkpred ( "a", {_x, Male} )  } ) );

//	predicate* goal = mkpred ( "a", { _y, Mortal } );
//	return prove ( mkrule ( 0, { goal } ), -1, cases );
//	cout << "evidence: " << evidence.size() << " items..." << endl;
//	cout << evidence << endl;
//	cout << evidence.size() << endl;
//	return evidence.size();
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
