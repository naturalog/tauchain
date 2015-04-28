// c++ port of euler (EYE) js reasoner
#include <deque>
#include "parsers.h"

#ifdef UBI
#include <UbigraphAPI.h>
#include <cmath>
#endif

bool test_reasoner() {
	evidence_t evidence, cases;
	pred_t Socrates = mk_res ( "Socrates" ), Man = mk_res ( "Man" ), Mortal = mk_res ( "Mortal" ), Morrtal = mk_res ( "Morrtal" ), Male = mk_res ( "Male" ), _x = mk_res ( "?x" ), _y = mk_res ( "?y" );
	cases["a"].push_back ( {{"a", {Socrates, Male}}, {}} );
	cases["a"].push_back ( {{"a", {_x, Mortal}}    , { {"a", {_x, Man}}, } } );
	cases["a"].push_back ( {
		{"a", {_x, Man}},
		{ {"a", {_x, Male}}, }
	} );

	bool p = prove ( pred_t {"a", {_y, Mortal}}, -1, cases, evidence );
	cout << "Prove returned " << p << endl;
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
		cout << endl << "---" << endl;
	}
	cout << "QED!" << endl;
	return evidence.size();
}

pred_t triple ( const string& s, const string& p, const string& o ) {
	return pred_t { p, { { s, {}}, { o, {}}}};
};

pred_t triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
};

//evidence_t prove ( , const psomap& query ) {
evidence_t prove ( const qlist& graph, const qlist& query, jsonld::rdf_db &kb ) {

	#ifdef UBI
	ubigraph_clear();
	#endif

	evidence_t evidence, cases;
	/*the way we store rules in jsonld is: graph1 implies graph2*/
	for ( const auto& quad : graph ) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		if ( p == "http://www.w3.org/2000/10/swap/log#implies" ) {
			//go thru all quads again, look for the implicated graph (rule head in prolog terms)
			for ( const auto &y : *kb[o] ) {
				rule_t rule;
				rule.head = triple ( *y );
				//now look for the subject graph
				for ( const auto& x : *kb[s] )
					rule.body.push_back ( triple ( *x ) );
				cases[rule.head.pred].push_back ( rule );
			}
		} else {
			rule_t r = { { p, { mk_res ( s ), mk_res ( o ) }}, {}};
			cout << ( string ) r << endl;
			cases[p].push_back ( r );
		}
	}
	rule_t goal;
	for ( auto q : query ) goal.body.push_back ( triple ( *q ) );
	ubi_add_facts ( cases );
	prove ( goal, -1, cases, evidence );
	return evidence;
}


void print_evidence ( evidence_t evidence ) {
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
		cout << endl << "---" << endl;
	}
}

const bool use_nquads = false;
