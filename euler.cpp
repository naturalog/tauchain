#include <string>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <deque>
using namespace std;

struct pred_t {
	string pred;
	vector<pred_t> args;
};

typedef map<string, pred_t> env_t;

struct rule_t {
	pred_t head;
	vector<pred_t> body;
};

struct s2 {
	rule_t src;
	env_t env;
};
typedef vector<s2> ground_t;

struct s1 {
	rule_t rule;
	int src, ind;
	s1* parent;
	env_t env;
	ground_t ground;
};

int builtin ( pred_t, s1 );
typedef map<string, vector<rule_t>> evidence_t;

pred_t evaluate ( pred_t t, env_t& env );
bool unify ( pred_t s, env_t senv, pred_t d, env_t denv, bool f );

template<typename ret_t>
ret_t prove ( pred_t goal, int maxNumberOfSteps ) {
	deque<s1> queue;
	s1 s = { goal, 0, 0, 0, env_t(), ground_t() };
	queue.emplace_back ( s );
	size_t step;
	evidence_t evidence;
	while ( queue.size() > 0 ) {
		s1 c = queue.pop_front();
		ground_t g = c.ground;
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) return "";
		if ( c.ind >= c.rule.body.size() ) {
			if ( !c.parent ) {
				rule_t tmp;
				pred_t tpred;
				for ( size_t i = 0; i < c.rule.body.size(); i++ ) {
					pred_t t = evaluate ( c.rule.body[i], c.env );
					evidence[t.pred].emplace_back ( tmp = {t, tpred = { "GND", c.ground} } );
				}
				continue;
			}
			s2 tmp;
			if ( c.rule.body.size() != 0 ) g.push_back ( tmp = {c.rule, c.env} );
			s1 r = {c.parent->rule, c.parent->src, c.parent->ind, c.parent->parent, c.parent->env, g};
			unify ( c.rule.head, c.env, r.rule.body[r.ind], r.env, true );
			r.ind++;
			queue.push_back ( r );
			continue;
		}
		pred_t t = c.rule.body[c.ind];
		size_t b = builtin ( t, c );
		if ( b == 1 ) {
			s2 tmp;
			rule_t rtmp;
			g.emplace_back ( tmp = { rtmp = { evaluate ( t, c.env ), vector<pred_t>() }, env_t() } );
			s1 r = {c.rule, c.src, c.ind, c.parent, c.env, g};
			r.ind++;
			queue.push_back ( r );
			continue;
		} else if ( !b ) continue;
		map<string, vector<rule_t>> cases;
		if ( cases.find ( t.pred ) == cases.end() ) continue;
		size_t src = 0;
		for ( size_t k = 0; k < cases[t.pred].size(); k++ ) {
			rule_t rl = cases[t.pred][k];
			src++;
			ground_t g = c.ground;
			s2 tmp;
			if ( rl.body.size() == 0 ) g.push_back ( tmp = {rl, vector<pred_t>() } );
			s1 r = {rl, src, 0, c, {}, g};
			if ( unify ( t, c.env, rl.head, r.env, true ) )
				for ( s1 ep = c; ; ep = ep.parent ) {
					if ( ep.src == c.src && unify ( ep.rule.head, ep.env, c.rule.head, c.env, false ) ) break;
					if ( !ep.parent ) {
						queue.push_front ( r );
						break;
					}
				}
		}
	}
}

bool unify ( pred_t s, env_t senv, pred_t d, env_t denv, bool f ) {
	if ( s.pred[0] == '?' ) {
		try {
			pred_t sval = evaluate ( s, senv );
			return unify ( sval, senv, d, denv, f );
		} catch ( ... ) {
			return true;
		}
	}
	if ( d.pred[0] == '?' )
		try {
			pred_t dval = evaluate ( d, denv );
			return unify ( s, senv, dval, denv, f );
		} catch ( ... ) {
			if ( f ) denv[d.pred] = evaluate ( s, senv );
			return true;
		}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
		for ( size_t i = 0; i < s.args.size(); i++ ) if ( !unify ( s.args[i], senv, d.args[i], denv, f ) ) return false;
		return true;
	}
	return false;
}

pred_t evaluate ( pred_t t, env_t& env ) {
	if ( t.pred[0] == '?' ) {
		auto it = env.find ( t.pred );
		if ( it != env.end() ) return evaluate ( it->second, env );
		else throw 0;
	} else if ( t.args.size() == 0 ) return t;
	else {
		pred_t tmp;
		vector<pred_t> n;
		for ( size_t i = 0; i < t.args.size(); ++i )
			try {
				n.push_back ( evaluate ( t.args[i], env ) );
			} catch ( ... ) {
				n.push_back ( tmp = { t.args[i].pred, vector<pred_t>() } );
			}
		return {t.pred, n};
	}
}

