/*
    euler.cpp

    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include "reasoner.h"
#include "parsers.h"
#include "rdf.h"

reasoner::reasoner() : GND ( &predicates[npredicates++].init ( dict.set ( "GND" ) ) ) {}

reasoner::~reasoner() {
	delete[] predicates;
	delete[] rules;
	delete[] frames;
}

rule& rule::init ( const predicate* h, predlist b ) {
	head = h;
	body = b;
	return *this;
}

frame& frame::init ( reasoner* r, const frame& f ) {
	//	if ( r->nframes >= max_frames ) throw "Buffer overflow";
	//	const frame& res =
	return init ( r, f.rul, f.ind, f.parent, f.substitution, f.ground );
	//	res.parent = f.parent;
	//	return res;
}

frame& frame::init ( reasoner* rs, const rule* _r, uint _ind, const frame* p, subst _s, ground_t _g ) {
	if ( rs->nframes >= max_frames ) throw "Buffer overflow";
	frame& f = rs->frames[rs->nframes++];
	f.rul = _r;
	f.ind = _ind;
	f.parent = p;
	f.substitution = _s;
	f.ground = _g;
	return f;
}

void reasoner::printkb() {
	static bool pause = false;
	cout << endl << "dumping kb with " << npredicates << " predicates, " << nrules << " rules and " << nframes << " frames. " << endl;
	cout << "predicates: " <<  endl;
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	cout << "rules: " << endl;
	for ( uint n = 0; n < nrules; ++n ) cout << rules[n] << endl;
	cout << "frames: " << endl;
	for ( uint n = 0; n < nframes; ++n ) cout << frames[n] << endl;
	if ( pause ) cout << "type <enter> to continue or <c><enter> to stop pausing...";
	cout << endl;
	if ( pause && getchar() == 'c' ) pause = false;
}

const predicate* reasoner::evaluate ( const predicate& t, const subst& sub ) {
	trace ( "\tEval " << t << " in " << sub << endl );
	if ( t.pred < 0 ) {
		auto it = sub.find ( t.pred );
		return it == sub.end() ? 0 : evaluate ( *it->second, sub );
	} else if ( t.args.empty() ) return &t;
	else {
		const predicate *p;
		predicate* r = &predicates[npredicates++].init ( t.pred );
		for ( auto x : t.args )
			r->args.emplace_back ( ( p = evaluate ( *x, sub ) ) ? p : &predicates[npredicates++].init ( x->pred ) );
		return r;
	}
}

bool reasoner::unify ( const predicate* _s, const subst& ssub, const predicate* _d, subst& dsub, bool f ) {
	if ( !_s && !_d ) {
		trace ( "Match two nulls." << endl );
		return true;
	}
	//	if ( !_s ) return _d && _d->pred >= 0; // ??
	//	if ( !_d ) return _s && _s->pred >= 0; // ??
	const predicate& s = *_s;
	const predicate& d = *_d;
	trace ( "\tUnify s: " << s << " in " << ( ssub ) << " with " << d << " in " << dsub << endl );
	//	if (s.pred == d.pred) trace("we have local pred match"<<endl);
	const predicate* p;
	if ( s.pred < 0 ) {
		if ( ( p = evaluate ( s, ssub ) ) ) return unify ( p, ssub, _d, dsub, f );
		else {
			trace ( "Match." << endl );
			return true;
		}
	}
	if ( d.pred >= 0 ) {
		if ( s.pred != d.pred || s.args.size() != d.args.size() ) return false;
		const predlist& as = s.args, ad = d.args;
		for ( auto sit = as.begin(), dit = ad.begin(); sit != as.end(); ++sit, ++dit )
			if ( !unify ( *sit, ssub, *dit, dsub, f ) )
				return false;
		trace ( "Match." << endl );
		return true;
	}
	p = evaluate ( d, dsub );
	if ( ( p = evaluate ( d, dsub ) ) ) return unify ( _s, ssub, p, dsub, f );
	if ( f ) dsub[d.pred] = evaluate ( s, ssub );
	trace ( "Match with subst: " << dsub << endl );
	return true;
}

void reasoner::evidence_found ( const frame& current_frame, evidence_t& evidence ) {
	for ( const predicate* x : current_frame.rul->body ) {
		const predicate* t = evaluate ( *x, current_frame.substitution );
		evidence[t->pred].emplace_back ( t, current_frame.ground );
	}
}

frame* reasoner::next_frame ( const frame& current_frame, ground_t& g ) {
	if ( !current_frame.rul->body.empty() ) g.emplace_front ( current_frame.rul, current_frame.substitution );
	frame& new_frame = frame::init ( this, *current_frame.parent );
	new_frame.ground = g;
	unify ( current_frame.rul->head, current_frame.substitution, new_frame.rul->body[new_frame.ind], new_frame.substitution, true );
	new_frame.ind++;
	return &new_frame;
}

void reasoner::match_rule ( frame& current_frame, const predicate& t, const rule& rl, deque<frame*>& queue ) {
	ground_t ground = current_frame.ground;
	if ( rl.body.empty() ) ground.emplace_back ( &rl, subst() );
	frame candidate_frame;
	candidate_frame.rul = &rl;
	candidate_frame.ind = 0;
	candidate_frame.parent = &current_frame;
	if ( unify ( &t, current_frame.substitution, rl.head, candidate_frame.substitution, true ) ) {
		trace ( "unification of rule " << rl << " from cases against " << t << " passed" << endl );
		const frame* ep = &current_frame;
		while ( ep->parent ) {
			ep = ep->parent;
			if ( ( ep->rul == current_frame.rul ) &&
			        unify ( ep->rul->head, ep->substitution, current_frame.rul->head, current_frame.substitution, false ) )
				break;
		}
		if ( !ep->parent ) queue.push_front ( &frame::init ( this, candidate_frame ) );
	} else {
		trace ( "unification of rule " << rl << " from cases against " << t << " failed" << endl );
	}
}

evidence_t reasoner::operator() ( rule* goal, int max_steps, cases_t& cases ) {
	trace ( "dict: " << dict.tostr() << endl );
	deque<frame*> queue;
	queue.emplace_back ( &frame::init ( this, goal ) );
	uint step = 0;
	evidence_t evidence;

	cout << "goal: " << *goal << endl << "cases:" << endl << cases << endl;
	while ( !queue.empty() && ++step ) {
		if ( max_steps != -1 && ( int ) step >= max_steps ) return evidence_t();
		frame& current_frame = *queue.front();
		queue.pop_front();
		ground_t g = current_frame.ground;
		trace ( current_frame << endl );
		if ( current_frame.ind >= current_frame.rul->body.size() ) {
			if ( !current_frame.parent ) evidence_found ( current_frame, evidence );
			else queue.push_back ( next_frame ( current_frame, g ) );
		} else {
			const predicate* t = current_frame.rul->body[current_frame.ind];
			int b = builtin ( t, current_frame ); // ( t, c );
			if ( b == 1 ) {
				g.emplace_back ( &rules[nrules++].init ( evaluate ( *t, current_frame.substitution ) ), subst() );
				frame& r = frame::init ( this, current_frame );
				r.ground = g;
				r.ind++;
				queue.push_back ( &r );
			} else if ( !b ) continue;
			auto it = cases.find ( t->pred );
			if ( it != cases.end() )
				for ( const rule* rl : it->second )
					match_rule ( current_frame, *t, *rl, queue );
		}

	}
	return evidence;
}

predicate* reasoner::mkpred ( string s, const vector<const predicate*>& v ) {
	return &predicates[npredicates++].init ( dict.has ( s ) ? dict[s] : dict.set ( s ), v );
}

rule* reasoner::mkrule ( const predicate* p, const vector<const predicate*>& v ) {
	return &rules[nrules++].init ( p, v );
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

evidence_t reasoner::operator() ( const qdb &kb, const qlist& query ) {
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
	printkb();
	return ( *this ) ( &goal, -1, cases );
}

bool reasoner::test_reasoner() {
	dict.set ( "a" );
	//	cout <<"dict:"<<endl<< dict.tostr() << endl;
	//	exit(0);
	evidence_t evidence;
	cases_t cases;
	typedef predicate* ppredicate;
	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {Socrates, Male} ) ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Mortal} ), { mkpred ( "a", {_x, Man } )  } ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Man   } ), { mkpred ( "a", {_x, Male} )  } ) );

	cout << "cases:" << endl << cases << endl;
	predicate* goal = mkpred ( "a", { _y, Mortal } );
	evidence = ( *this ) ( mkrule ( 0, { goal } ), -1, cases );
	cout << "evidence: " << evidence.size() << " items..." << endl;
	cout << evidence << endl;
	cout << "QED!" << endl;
	cout << evidence.size() << endl;
	return evidence.size();
}

float degrees ( float f ) {
	static float pi = acos ( -1 );
	return f * 180 / pi;
}


int reasoner::builtin ( const predicate* t_, frame& f ) {
	if ( !t_ ) return -1;
	const predicate& t = *t_;
	string p = dict[t.pred];
	if ( p == "GND" ) return 1;
	if ( t.args.size() != 2 ) throw runtime_error ( "builtin expects to get a predicate with spo" );
	const predicate* t0 = evaluate ( **t.args.begin(), f.substitution );
	const predicate* t1 = evaluate ( **++t.args.begin(), f.substitution );
	if ( p == "log:equalTo" ) return ( ( t0 && t1 && t0->pred == t1->pred ) ? 1 : 0 );
	else if ( p == "log:notEqualTo" ) return ( t0 && t1 && t0->pred != t1->pred ) ? 1 : 0;
	else if ( p == "math:absoluteValue" ) {
		if ( t0 && t0->pred > 0 )
			return ( unify ( mkpred ( tostr ( fabs ( stof ( dict[t0->pred] ) ) ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	}
#define FLOAT_BINARY_BUILTIN(x, y) \
  else if (p == x) \
      return \
        (t0 && t0->pred > 0 && \
      	(unify(mkpred(tostr(y(stof(dict[t0->pred])))), f.substitution, t.args[1], f.substitution, true))) || \
        (t1 && t1->pred > 0 && \
      	(unify(mkpred(tostr(y(stof(dict[t0->pred])))), f.substitution, t.args[0], f.substitution, true))) \
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
		float a = stof ( dict[ ( evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred )] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a -= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		return ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	} else if ( p == "math:exponentiation" && t0 ) {
		float a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; ti && !ti->args.empty(); ti = ti->args[1] ) a = pow ( a, stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] ) );
		return ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	} else if ( p == "math:integerQuotient" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a /= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		return ( unify ( mkpred ( tostr ( floor ( a ) ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	} else if ( p == "math:negation" ) {
		if ( t0 && t0->pred >= 0 ) {
			auto a = -stof ( dict[t0->pred] );
			if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		} else if ( t1 && t1->pred >= 0 ) {
			auto a = -stof ( dict[t1->pred] );
			if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[0], f.substitution, true ) ) return 1;
		} else return 0;
	} else if ( p == "math:product" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a *= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "math:quotient" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a /= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "math:remainder" && t0 ) {
		auto a = stoi ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a %= stoi ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "math:rounded" ) {
		if ( t0 && t0->pred >= 0 ) {
			float a = round ( stof ( dict[t0->pred] ) );
			if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		} else return 0;
	} else if ( p == "math:sum" && t0 ) {
		float a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a += stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "rdf:first" && t0 && dict[t0->pred] == "." && !t0->args.empty() )
		return ( unify ( t0->args[0], f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	else if ( p == "rdf:rest" && t0 && dict[t0->pred] == "." && !t0->args.empty() )
		return ( unify ( t0->args[1], f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	else if ( p == "a" && t1 && dict[t1->pred] == "rdf:List" && t0 && dict[t0->pred] == "." ) return 1;
	else if ( p == "a" && t1 && dict[t1->pred] == "rdfs:Resource" ) return 1;
	return -1;
}
