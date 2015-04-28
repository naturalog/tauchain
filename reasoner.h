// c++ port of euler (EYE) js reasoner
#include <deque>
#include "parsers.h"

#ifdef UBI
#include <UbigraphAPI.h>
#include <cmath>
#endif

struct pred_t {
	string pred;
	vector<pred_t> args;

	ostream& write ( ostream& o ) const {
		o << pred;
		if ( args.size() ) {
			o << "(";
			for ( auto a = args.cbegin();; ) {
				a->write ( o );
				if ( ++a != args.cend() ) o << ", ";
				else return o << ")";
			}
		}
		return o;
	}

	operator string() const {
		stringstream ss;
		write ( ss );
		return ss.str();
	}
};

int _indent = 0;

string indent() {
	stringstream ss;
	for ( int i = 0; i < _indent; i++ ) ss << "##";
	return ss.str();
}

typedef map<string, pred_t> env_t;
typedef std::shared_ptr<env_t> penv_t;

ostream& operator<< ( ostream& o, env_t const& r ) {
	o << "env of size " << r.size() << "{";
	for ( auto rr : r ) o << rr.first << ": " << ( string ) rr.second << "; ";
	return o << "}";
}

struct rule_t {
	pred_t head;
	vector<pred_t> body;
#ifdef UBI
	int ubi_node_id;
#endif
	string tostring() const {
		stringstream o;
		o << ( string ) head;
		if (body.size()) 
		{
			o << " :- ";
			for ( auto x : body ) x.write ( o );
		}
		else
			o << ".";
		return o.str();
	}
	operator string() const {
		return tostring();
	}
};

typedef map<string, vector<rule_t>> evidence_t;

struct rule_env {
	rule_t src;
	penv_t env;
	operator string() const {
		stringstream o;
		o << ( string ) src << " {" << *env << "} ";
		return o.str();
	}
};
typedef vector<rule_env> ground_t;
typedef std::shared_ptr<ground_t> pground_t;

pground_t aCopy ( pground_t f ) {
	pground_t r = make_shared<ground_t>();
	*r = *f;
	return r;
}

ostream& operator<< ( ostream& o, ground_t const& g ) {
	for ( auto gg : g ) o << "_" << ( string ) gg << "_";
	return o;
}

struct proof_trace_item {
	rule_t rule;
	int src, ind; // source of what, index of what?
	std::shared_ptr<proof_trace_item> parent;
	std::shared_ptr<env_t> env;
	std::shared_ptr<ground_t> ground;
#ifdef UBI
	int ubi_node_id;
#endif
	operator string() const {
		stringstream o;
		o << "<<" << ( string ) rule << src << "," << ind << "(";
		if ( parent ) o << ( string ) ( *parent );
		o << ") {env:" << ( *env ) << "}[[ground:" << ( *ground ) << "]]";
		return o.str();
	}
};
typedef std::shared_ptr<proof_trace_item> ppti;



void ubi(const ppti i)
{
#ifdef UBI
	i->ubi_node_id = ubigraph_new_vertex();

	ubigraph_set_vertex_attribute(i->ubi_node_id, "fontsize", "20");
	ubigraph_set_vertex_attribute(i->ubi_node_id, "label", ((string)i->rule).c_str());
	ubigraph_set_vertex_attribute(i->ubi_node_id, "fontcolor", "#ffffff");

	if (i->parent)
	{
		ubigraph_set_vertex_attribute(i->ubi_node_id, "color", "#ffffff");
		ubigraph_set_vertex_attribute(i->ubi_node_id, "shape", "icosahedron");

		int e = ubigraph_new_edge(i->parent->ubi_node_id, i->ubi_node_id);
		ubigraph_set_edge_attribute(e, "color", "#ffffff");
		ubigraph_set_edge_attribute(e, "width", "5.0");
		ubigraph_set_edge_attribute(e, "oriented", "true");
	}
	else
	{
		ubigraph_set_vertex_attribute(i->ubi_node_id, "color", "#ff0000");
		ubigraph_set_vertex_attribute(i->ubi_node_id, "shape", "octahedron");
	}
	if (i->rule.ubi_node_id)
		int e = ubigraph_new_edge(i->ubi_node_id, i->rule.ubi_node_id);
#endif
}

void ubi_add_facts(evidence_t &cases)
{
#ifdef UBI
	vector<int> ids;
	for ( auto c : cases ) 
		for ( auto r : c.second ) 
			if ( !r.body.size() )
			{
				ids.push_back((
					r.ubi_node_id = ubigraph_new_vertex()));
				ubigraph_set_vertex_attribute(r.ubi_node_id, "fontsize", "18");
				ubigraph_set_vertex_attribute(r.ubi_node_id, "label", ((string)r).c_str());
				ubigraph_set_vertex_attribute(r.ubi_node_id, "fontcolor", "#ffffff");
			}

	int w = sqrt(ids.size()) + 1;
	for (int x = 0; x < w; x++)
	{
		int h = ids.size()/w+1;
		
		for (int y = 0; y < h; y++)
		{
			if ((x*h+y) == ids.size())
				return;

			cout << ids.size() << " facts, w:" << w << 
				", h:" << h << ", x:"<<x << ", y:"<<y <<endl;

			int id = ids[x*h+y];
			if (x>0)
			{
				int e = ubigraph_new_edge(id, ids[(x-1)*h+y]);
			}
			if(y>0)
			{
				int e = ubigraph_new_edge(id, ids[x*h+y-1]);
			}
		}
	}
#endif
}


int builtin ( pred_t, proof_trace_item ) {
	/*
	    1:it unified
	    -1:no such builtin
	    0:it didnt unify
	*/
	trace ( "NO BEES yet PLZ!" << endl );
	return -1;
}

pred_t evaluate ( const pred_t t, const penv_t env );
bool unify ( const pred_t s, const penv_t senv, const pred_t d, const penv_t denv );

pground_t gnd = make_shared<ground_t>();

bool prove ( rule_t goal, int maxNumberOfSteps, evidence_t& cases, evidence_t& evidence ) {
	/*
	    please write an outline of this thing:)
	*/
	int step = 0;
	deque<ppti> queue;
	ppti s = make_shared<proof_trace_item> ( proof_trace_item { goal, 0, 0, 0, make_shared<env_t>(), gnd } ); //TODO: don't deref null parent ;-)//done?
	queue.emplace_back ( s ); ubi(s);
	//queue.push_back(s);
	trace (  "Goal: " << ( string ) goal << endl );
	while ( queue.size() > 0 ) {
		trace (  "=======" << endl );
		ppti c = queue.front();
		trace (  "  c: " << ( string ) ( *c ) << endl );
		queue.pop_front();
		pground_t g = aCopy ( c->ground );
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) {
			trace (  "TIMEOUT!" << endl );
			return false;
		}
		trace ( "c.ind: " << c->ind << endl << "c.rule.body.size(): " << c->rule.body.size() << endl ); //in step 1, rule body is goal
		// all parts of rule body succeeded...(?)
		if ( ( size_t ) c->ind >= c->rule.body.size() ) {
			if ( !c->parent ) {
				trace ( "no parent!" << endl );
				for ( size_t i = 0; i < c->rule.body.size(); i++ ) {
					pred_t t = evaluate ( c->rule.body[i], c->env );
					rule_t tmp = {t, {{ "GND", {}}} };
					trace (  "Adding evidence for " << ( string ) t.pred << ": " << ( string ) tmp << endl );
					evidence[t.pred].push_back ( tmp );
				}
				continue;
			}
			trace ( "Q parent: " );
			if ( c->rule.body.size() != 0 ) g->push_back ( {c->rule, c->env} );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {c->parent->rule, c->parent->src, c->parent->ind, c->parent->parent, make_shared<env_t>(*c->parent->env) , g} );
			unify ( c->rule.head, c->env, r->rule.body[r->ind], r->env );
			r->ind++;
			trace (  ( string ) ( *r ) << endl );
			queue.push_back ( r ); //ubi(r);
			continue;
		}
		trace ( "Done q" << endl );
		pred_t t = c->rule.body[c->ind];
		size_t b = builtin ( t, *c );
		if ( b == 1 ) {
			g->emplace_back ( rule_env { { evaluate ( t, c->env ), vector<pred_t>() }, make_shared<env_t>() } );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {c->rule, c->src, c->ind, c->parent, c->env, g} );
			r->ind++;
			queue.push_back ( r ); ubi(r);
			continue;
		} else if ( b == 0 )   // builtin didnt unify
			continue; // else there is no such builtin, continue...

		trace ( "Checking cases..." << endl );
		trace ( "looking for case " << t.pred << endl );
		if ( cases.find ( t.pred ) == cases.end() ) {
			trace ( "No Cases(no such predicate)!" << endl );
			trace ( "available cases' keys: " << endl );
			for ( auto x : cases ) trace ( x.first << endl );
			continue;
		}
		size_t src = 0;
		//for each rule with the predicate we are trying to prove...
		for ( rule_t rl : cases[t.pred] ) {
			src++;
			pground_t g = aCopy ( c->ground );
			trace (  "Check rule: " << ( string ) rl << endl );
			if ( rl.body.size() == 0 ) g->push_back ( { rl, make_shared<env_t>() } ); //its a conditionless fact
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {rl, ( int ) src, 0, c, make_shared<env_t>(), g} );// why already here and not later?
			//rl could imply our rule...
			if ( unify ( t, c->env, rl.head, r->env ) ) {
				ppti ep = c;
				while ( ( ep = ep->parent ) ) {
					trace ( "  ep.src: " << ep->src << endl << "  c.src: " << c->src << endl );
					if ( ep->src == c->src && unify ( ep->rule.head, ep->env, c->rule.head, c->env ) ) {
						trace ( "  ~~ match ~~ " << endl );
						break;
					}
					trace ( "  ~~  ~~  ~~" << endl );
				}
				if ( !ep ) {
					trace ( "Adding to queue: " << ( string ) ( *r ) << endl << flush );
					queue.push_front ( r ); ubi(r);
				} else trace ( "didn't reach top" << endl );
				trace ( "Done euler loop" << endl );
			} else trace ( "No loop here" << endl );
		}
		trace ( "done rule checks, looping" << endl );
	}
	return false;
}

bool prove ( pred_t goal, int maxNumberOfSteps, evidence_t& cases, evidence_t& evidence ) {
	return prove ( rule_t {goal, { goal } }, maxNumberOfSteps, cases, evidence );
}
bool unify ( const pred_t s, const penv_t senv, const pred_t d, const penv_t denv ) {
	trace ( indent() << "Unify:" << endl << flush << indent() << "  s: " << ( string ) s << " in " << ( *senv ) << endl << flush;
	        cout << indent() << "  d: " << ( string ) d << " in " << ( *denv ) << endl << flush );
	if ( s.pred[0] == '?' ) {
		trace ( indent() << "  source is var" << endl << flush );
		try {
			pred_t sval = evaluate ( s, senv );
			_indent++;
			bool r = unify ( sval, senv, d, denv );
			_indent--;
			return r;
		} catch (int n) {
			if (n) { bt(); throw std::runtime_error("Error during unify"); }
			if ( _indent ) _indent--;
			trace ( indent() << " Match(free var)!" << endl );
			return true;
		} catch (exception ex) { bt(); throw ex; }
		catch (...) { bt(); throw std::runtime_error("Error during unify"); }
	}
	if ( d.pred[0] == '?' ) {
		trace ( indent() << "  dest is var" << endl << flush );
		try {
			pred_t dval = evaluate ( d, denv );
			trace ( _indent++ );
			bool b = unify ( s, senv, dval, denv );
			trace ( _indent-- );
			return b;
		} catch ( int n) {
			if (n) { bt(); throw std::runtime_error("Error during unify"); }
			( *denv ) [d.pred] = evaluate ( s, senv );
			trace ( _indent-- );
			trace (  indent() << " Match!(free var)" << endl );
			return true;
		} catch (exception ex) { throw ex; }
		catch (...) { bt(); throw std::runtime_error("Error during unify"); }
	}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
		trace ( indent() << "  Comparison:" << endl << flush );
		for ( size_t i = 0; i < s.args.size(); i++ ) {
			_indent++;
			if ( !unify ( s.args[i], senv, d.args[i], denv ) ) {
				_indent--;
				trace ( indent() << "    " << ( string ) s.args[i] << " != " << ( string ) d.args[i] << endl << flush );
				return false;
			}
			_indent--;
			trace ( indent() << "    " << ( string ) s.args[i] << " == " << ( string ) d.args[i] << endl << flush );
		}
		trace ( indent() << "  Equal!" << endl << flush );
		return true;
	}
	trace ( indent() << " No match" << endl << flush );
	return false;
}

pred_t evaluate ( const pred_t t, const penv_t env ) {
	trace ( indent() << "Eval " << ( string ) t << " in " << ( *env ) << endl );
	if ( t.pred[0] == '?' ) {
		trace (  "(" << ( string ) t << " is a var..)" << endl );
		auto it = env->find ( t.pred );
		if ( it != env->end() ) return evaluate ( it->second, env );
		else throw int(0);
	} else if ( t.args.size() == 0 ) return t;
	vector<pred_t> n;
	for ( size_t i = 0; i < t.args.size(); ++i ) {
		try {
			n.push_back ( evaluate ( t.args[i], env ) );
		} catch (int k) {
			if (k) { bt(); throw std::runtime_error("Uncaught exception during evaluate()."); }
			n.push_back ( { t.args[i].pred, vector<pred_t>() } );
		} catch (exception ex) { bt(); throw ex; }
		catch (...) { bt(); throw std::runtime_error("Uncaught exception during evaluate()."); }
	}
	return {t.pred, n};
}

inline pred_t mk_res ( string r ) {
	return {r, {}};
}

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

//evidence_t prove ( const psomap& kb, const psomap& query ) {
evidence_t prove ( const qlist& kb, const qlist& query ) {

	#ifdef UBI
	ubigraph_clear();
	#endif

	evidence_t evidence, cases;
	/*the way we store rules in jsonld is: graph1 implies graph2*/
	for ( const auto& quad : kb ) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		if ( p == "http://www.w3.org/2000/10/swap/log#implies" ) {
			rule_t rule;
			//go thru all quads again, look for the implicated graph (rule head in prolog terms)
			for ( const auto& y : kb )
				if ( y->graph->value == o ) {
					rule.head = triple ( *y );
					//now look for the subject graph
					for ( const auto& x : kb )
						if ( x->graph->value == s )
							rule.body.push_back ( triple ( *x ) );
					cases[p].push_back ( rule );
				}
		} 
		else
		{
			rule_t r = { { p, { mk_res ( s ), mk_res ( o ) }}, {}};
			cout << (string)r << endl;
			cases[p].push_back (r);
		}
	}
	rule_t goal;
	for ( auto q : query ) goal.body.push_back ( triple ( *q ) );
	ubi_add_facts(cases);
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
