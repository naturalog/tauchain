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
	else { c.str = p; kb.emplace(c); }
}

word rule::crds(int rl) {
	word r;
	if (head) head->crds(r), r = push_front(r, -1);
	FORZ(n,body.size()) {
		word k;
		body[n]->t->crds(k, crd(0), n+1);
		r.insert(k.begin(), k.end());
	}
	return push_front(r, rl);
}

struct refutation {}; 
struct literal_clash : public refutation {};

struct pfds { // prefix free data structure. also throws on literal merge trial
	struct item {
		bool lit;
		pcch c, str; // c is coord
		item *rep = 0;
		uint rank = 0;
		item(pcch c, pcch str) : c(c), str(str), lit(*str != '?') {}
	};
	umap<pcch, item*> g;
	item* makeset(pcch c, pcch str) {
		//check pf
		auto it = g.find(c);
		if (it != g.end()) return it->second;
		return g.emplace(c, new item(c, str)).first->second;

	}
	item* merge(item *x, item *y) throw(refutation) {
		while (x->rep) (x = x->rep)->rep = x->rep->rep;
		while (y->rep) (y = y->rep)->rep = y->rep->rep;
		if (x->lit) {
			if (y->lit) {
				if (x->str != y->str) throw literal_clash();
				return x;
			} else y->rep = x, ++x->rank;
		} else if (y->lit || x->rank < y->rank) x->rep = y, ++y->rank;
		else y->rep = x, ++x->rank;
	}
};

struct wrd {
	int **c;
	pcch *str;
	uint sz;
	wrd(const word& w) {
		sz = w.size();
		c = new int*[sz];
		str = new pcch[sz];
		uint i = 0, j;
		for (ccrd& r:w) {
			c[i] = new int[r.c.size()+1], j = 0;
			str[i] = r.str, c[i][r.c.size()] = 0;
			for (int x : r.c) c[i][j++] = x; ++i;
		}
	}
	void refute();
};

template<typename T> void range(T **a, uint sz, uint off, T i, uint &f, uint &l) {
	uint c = sz / 2;
	for (f = 0, l = sz; f != l; c = (f+l)/2) {
		if (*(a[c]+off) < i) l = c;
		else if (*(a[c]+off) > i) f = c;
	}
}

int ccmp::operator()(ccrd& x, ccrd& y) {
	auto ix = x.c.begin(), ex = x.c.end();
	auto iy = y.c.begin(), ey = y.c.end();
	while (ix != ex && iy != ey)
		if (*ix != *iy) return *ix < *iy ? 1 : -1;
		else ++ix, ++iy;
	return 0;
}

#define os_each(x, y) for(auto x:y)os<<x
word push_front(cword& t, int i){word r;for(auto x:t){x.c.push_front(i);r.insert(x);}return r;}
ostm& operator<<(ostm& os,cword& w){os_each(c,w);return os<<endl;}
ostm& operator<<(ostm& os,ccrd& c) {os_each(i,c.c)<<':';
if(c.str)os<<'('<<c.str<<')';return os<<endl; }
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
ostm& operator<<(ostm& os,const wrd& w){ FORZ(n,w.sz){int *i=w.c[n];while(*i)os<<*i++<<':';
if(w.str[n])os<<w.str[n];os<<endl;}return os;}

void print(const ast& st) {
	dout << "rules: " << st.rules.size() << endl;
	uint n = 0;
	word kb;
	for (auto r : st.rules) {
		word k = r->crds(++n);
		for (auto x : k) kb.insert(x);
	}
	dout << kb << endl;
	for (auto r : st.rules) *r >> dout << endl;
	dout << wrd(kb) << endl;
}

int main() {
//	strings_test();
	ast st;
	st.terms.push_back(new term("GND"));
	readdoc(false, &st);
	//print();
//	din.readdoc(true);
	print(st);
}
