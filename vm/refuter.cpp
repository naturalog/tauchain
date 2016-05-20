#include "ast.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <unordered_map>
#define umap std::unordered_map
typedef ast::rule rule;
typedef ast::term term;
#define mkterm(x) new term(x)
#define FORZ(x, z) for (uint e__LINE__ = z, x = 0; x != e__LINE__; ++x)
#define FOR(x, y, z) for (auto e__LINE__ = z, x = y; x != e__LINE__; ++x)
template<typename T>
struct onret { T f; onret(T f):f(f){} ~onret(){f();} };

void ast::term::crds(word &kb, crd c, int k) const {
	if (k != -1) c.c.push_back(k);
	if (!p) FORZ(n,sz) args[n]->crds(kb, c, n);
	else { c.str = p; kb.push_back(c); }
}

word rule::crds(int rl) {
	word r;
	if (head) head->crds(r), push_front(r, -1);
	FORZ(n,body.size()) {
		word k;
		body[n]->t->crds(k, crd(0, 0), n+1);
		for (auto x : k) r.push_back(x);
	}
	return push_front(r, rl), r;
}

template<typename T>
struct pfds { // prefix free disjoint sets. represents all kb and throws all refutations
	struct item {
		lary<T> *c; // coords
		pcch str;
		bool lit;
		item *rep = 0;
		uint rank = 0;
		item(lary<T> *c, pcch str, pcch rst = 0) : c(c), str(str), lit(*str != '?') {}
		T& operator[](uint n) { return (*c)[n]; }
		const T& operator[](uint n) const { return (*c)[n]; }
	};
	std::map<lary<T>*, item*> g;
	item* makeset(lary<T> *c, pcch str) throw(prefix_clash, literal_clash) {
		item *i;
		T e = (*c)[0];
		for (auto it=g.upper_bound(c); (*(i = it->second))[0]==e; ++it)
			if (!c->cmp(*i->c)) return i;
			else if (i->str!=c->str && !i->var && !c->var)
				throw literal_clash();
		return g.emplace(c, new item(c, str)).first->second;

	}
	void merge(item *x, item *y) throw(refutation) {
		while (x->rep) (x = x->rep)->rep = x->rep->rep;
		while (y->rep) (y = y->rep)->rep = y->rep->rep;
		if (x->lit && y->lit) {if (x->str!=y->str) throw literal_clash();}
		else if (y->lit || x->rank < y->rank) x->rep = y, ++y->rank;
		else y->rep = x, ++x->rank;
	}
};
#define os_each(x, y) for(auto x:y)os<<x
void push_front(word& t, int i){word r;for(auto x:t)x.c.push_front(i),r.push_back(x);t=r;}
//ostm& operator<<(ostm& os,cword& w){os_each(c,w);return os<<endl;}
//ostm& operator<<(ostm& os,ccrd& c) {os_each(i,c.c)<<':';
//if(c.str)os<<'('<<c.str<<')';return os<<endl; }
ostm &term::operator>>(ostm &os)const{if(p)return os<<p;os<<'(';
auto a=args;while(*a)**a++>>os<<' ';return os<<')';}
term::term(term** _args,uint sz):p(0),args(new term*[sz+1]),sz(sz){
FORZ(n,sz)args[n]=_args[n];args[sz]=0;} 
term::term(pcch v):p(ustr(v)),sz(0){}term::~term(){if(!p&&args)delete[]args;} 
rule::premise::premise(cterm *t):t(t){} 
rule::rule(cterm *h,const body_t &b):head(h),body(b){}
template<typename T> inline vector<T> singleton(T x){ vector<T>r;r.push_back(x);return x;} 
rule::rule(cterm *h,cterm *b):head(h),body(singleton(new premise(b))){}
rule::rule(const rule &r) : head(r.head), body(r.body) {}
//ostm& operator<<(ostm& os,const wrd& w){ FORZ(n,w.sz){int *i=w.c[n];while(*i)os<<*i++<<':'; if(w.str[n])os<<w.str[n];os<<endl;}return os;}

void print(const ast& st) {
	dout << "rules: " << st.rules.size() << endl;
	uint n = 0;
	word kb;
	for (auto r : st.rules) {
		word k = r->crds(++n);
		for (auto x : k) kb.push_back(x);
	}
//	dout << kb << endl;
	for (auto r : st.rules) *r >> dout << endl;
//	dout << wrd(kb) << endl;
}

int main() {
//	strings_test();
	ast st;
	st.terms.push_back(new term("GND"));
	readdoc(false, &st);
	//print();
//	din.readdoc(true);
	print(st);
	pfds<int> i;
	lary<int> l(1);
}
