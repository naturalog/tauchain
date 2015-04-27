// Euler proof mechanism -- Jos De Roo
//version = '$Id: Euler4.js 1398 2007-07-20 16:41:33Z josd $'
#include<vector>
#include<cstdlib>
#include<cmath>
#include<map>
#include<stdexcept>
using namespace std;
typedef size_t pid;

struct predicate {
	int pred = 0;
	vector<pid> args;
};

const size_t max_preds = 1024 * 1024;
predicate preds[max_preds];
size_t npreds = 0;

pid mkpred ( int p, vector<pid> a ) {
	preds[npreds].pred = p;
	preds[npreds++].args = a;
	return npreds - 1;
}
typedef map<int, pid> subst;
//pid mkpred(pid p, vector<pid> a) { return mkpred(preds[p].pred, a); }

pid eval ( const pid _t, const subst s ) {
	const predicate& t = preds[_t];
	if ( t.pred < 0 ) {
		auto it = s.find ( t.pred );
		return it == s.end() ? mkpred ( 0, {} ) : eval ( it->second, s );
	} else if ( !t.args.size() == 0 ) return _t;
	pid _r = mkpred ( t.pred, {} );
	predicate& r = preds[_r];
	pid a;
	for ( pid x : t.args ) r.args.push_back ( ( a = eval ( x, s ) ) ? mkpred ( preds[x].pred, {} ) : a );
	return _r;
}

bool unify ( const pid _s, const subst senv, const pid _d, subst& denv, const void* f, const pid __d = 0 ) {
	if (!_s) return true;
	if (!_d) {
		if ( f ) { 
			if (!__d) throw logic_error("wrt unify's __d");
			denv[preds[__d].pred] = eval ( _s, senv );
		}
		return true;
	}
	const predicate &s = preds[_s], &d = preds[_d];
	if ( s.pred < 0 ) return unify ( eval ( _s, senv ), senv, _d, denv, f );
	if ( d.pred < 0 ) return unify ( _s, senv, eval ( _d, denv ), denv, f, _d );

	if ( s.pred != d.pred || s.args.size() != d.args.size() ) return false;

	for ( size_t i = 0; i < s.args.size(); ++i ) 
		if ( !unify ( s.args[i], senv, d.args[i], denv, f ) ) 
			return false;
	return true;
}
/*
    function prove ( goal, maxNumberOfSteps ) {
	var queue = [ {rule:goal, src:0, ind:0, parent:null, env:{}, ground: []}]
	            if ( typeof ( evidence ) == 'undefined' ) evidence = {}
		                    if ( typeof ( step ) == 'undefined' ) step = 0
			while ( queue.length > 0 ) {
				var c = queue.pop()
				        if ( typeof ( trace ) != 'undefined' ) document.writeln ( 'POP QUEUE\n' + JSON.stringify ( c.rule ) + '\n' )
					        var g = aCopy ( c.ground )
					                step++
					                if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) return ''
						if ( c.ind >= c.rule.body.length ) {
							if ( c.parent == null ) {
								for ( var i = 0; i < c.rule.body.length; i++ ) {
									var t = evaluate ( c.rule.body[i], c.env )
									        if ( typeof ( evidence[t.pred] ) == 'undefined' ) evidence[t.pred] = []
										                evidence[t.pred].push ( {head: t, body: [{pred:'GND', args:c.ground}]} )
									}
								continue
							}
							if ( c.rule.body.length != 0 ) g.push ( {src: c.rule, env: c.env} )
    var r = {rule:
								{head: c.parent.rule.head, body: c.parent.rule.body}, src:
    c.parent.src, ind:
								c.parent.ind,
    parent:
								c.parent.parent != null ? new copy ( c.parent.parent ) : null, env : new copy ( c.parent.env ), ground : g
							}
							unify ( c.rule.head, c.env, r.rule.body[r.ind], r.env, true )
							r.ind++
							queue.push ( r )
							if ( typeof ( trace ) != 'undefined' ) document.writeln ( 'PUSH QUEUE\n' + JSON.stringify ( r.rule ) + '\n' )
								continue
							}
				var t = c.rule.body[c.ind]
				        var b = builtin ( t, c )
				if ( b == 1 ) {
					g.push ( {src: {head: evaluate ( t, c.env ), body: []}, env: {}} )
					var r = {rule: {head: c.rule.head, body: c.rule.body}, src: c.src, ind: c.ind, parent: c.parent, env: c.env, ground: g}
					        r.ind++
					        queue.push ( r )
					        if ( typeof ( trace ) != 'undefined' ) document.writeln ( 'PUSH QUEUE\n' + JSON.stringify ( r.rule ) + '\n' )
						        continue
					} else if ( b == 0 ) continue
					if ( cases[t.pred] == null ) continue
						var src = 0
						for ( var k = 0; k < cases[t.pred].length; k++ ) {
							var rl = cases[t.pred][k]
							         src++
							         var g = aCopy ( c.ground )
							                 if ( rl.body.length == 0 ) g.push ( {src: rl, env: {}} )
								                 var r = {rule: rl, src: src, ind: 0, parent: c, env: {}, ground: g}
								if ( unify ( t, c.env, rl.head, r.env, true ) ) {
									var ep = c  // euler path
									         while ( ep = ep.parent ) if ( ep.src == c.src && unify ( ep.rule.head, ep.env, c.rule.head, c.env, false ) ) break
											if ( ep == null ) {
												queue.unshift ( r )
												if ( typeof ( trace ) != 'undefined' ) document.writeln ( 'EULER PATH UNSHIFT QUEUE\n' + JSON.stringify ( r.rule ) + '\n' )
												}
								}
						}
			}
    }

*/
