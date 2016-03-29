#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <map>
#include <assert.h>

#define has(x,y) ((x).find(y) != (x).end())             


//well, the problem is whether or not the map knows if the ruleterms are the same,
//because they're composed of terms that are still different

template<typename T>
struct sp { // smart pointer
	T* p = 0;
	int* r = 0;
	sp() {}
	sp(T* v) : p(v) 			{ *(r = new int) = 1; }
	sp(const sp<T>& s) : p(s.p), r(s.r) 	{ if (r) ++*r; }

	~sp() { if (r && !--*r) delete p , delete r; }

	T& operator*()  { return *p; } 
	T* operator->() { return p; }
    
	sp<T>& operator=(const sp<T>& s) {
		if (this == &s) return *this;
		if (r && !--*r) delete p , delete r;
		p = s.p;
		if ((r = s.r)) ++*r;
		return *this;
	}
};
typedef sp<struct frame> sp_frame;

const size_t chunk = 4;
template<typename T, bool ispod = std::is_pod<T>::value>
class vector {
protected:
	T* a;
	const size_t szchunk = chunk * sizeof(T);
	size_t n, c;
public:
	explicit vector() : a(0),n(0),c(0) {}
	vector(const vector<T>& t) : a(0),n(0),c(0) { copyfrom(t); }
	vector<T>& operator=(const vector<T>& t) { copyfrom(t); return *this; }

	typedef T* iterator;
	T operator[](size_t k) const { return a[k]; }
	size_t size() const	{ return n; }
	T* begin() const	{ return a; }
	T* end() const		{ return a ? &a[n] : 0; }
	void clear()		{
		n = 0; c = 0;
		if (!a) return;
		free(a);
		a = 0;
	}
	bool empty() const	{ return !a; }
	~vector()		{ clear(); }
	iterator back()		{ return n ? &a[n-1] : 0; }
	T& push_back(const T& t) {
		if (!(n % chunk))
			a = (T*)realloc(a, szchunk * ++c);
		if (ispod) a[n] = t;
		else new (&a[n])(T)(t);
		return a[n++];
	}
protected:	
	void copyfrom(const vector<T>& t) {
		clear();
		if (!(n = t.n)) return; 
		memcpy(a = (T*)malloc(t.c * szchunk), t.a, (c = t.c) * szchunk);
//		if (!ispod)
//			for (size_t k = 0; k < n; ++k)
//				new (&a[k])(T);
	}
};

template<typename K, typename V>
struct mapelem {
	K first;
	V second;
	mapelem(){}
	mapelem(const mapelem& m) : first(m.first), second(m.second) {}
	mapelem(const K& _k, const V& _v) : first(_k), second(_v) {} 
};

template<typename K, typename V>
struct map : public vector<mapelem<K, V> > {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V> > base;
public:
	map(const vector<vtype>& t) { copyfrom(t); }
	map() : base() {}
	map(const map<K, V>& _s) : base()      	{ base::copyfrom(_s); }
	map<K, V>& operator=(const map<K, V>& m){ base::copyfrom(m); return *this; }
//	const V& operator[](const K& k) const	{ mapelem<K, V>* z = find(k); return z->second; } // ? z->second : set(k, V()); }
	V& operator[](const K& k)		{ mapelem<K, V>* z = find(k); return z ? z->second : set(k, V()); }
	V& set(const K& k, const V& v) {
		vtype* z = find(k);
		if (z) { return z->second = v; }
		return base::push_back(vtype(k, v)).second;
//		qsort(base::a, base::n, sizeof(vtype), compare);
	}
	typedef vtype* iterator;
	iterator find(const K& k) const {
		if (!base::n) return 0;
		for (vtype& x : *this) if (x.first == k) return &x;
		return 0;
//		vtype v(k, V());
//		vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare);
//		return z;
	}
private:
	static int compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }
};

typedef std::wstring string;
void trim(string& s) {
	string::iterator i = s.begin();
	while (iswspace(*i)) {
		s.erase(i);
		i = s.begin();
	}
	size_t n = s.size();
	if (n) {
		while (iswspace(s[--n]));
		s = s.substr(0, ++n);
	}
}

typedef int resid;
std::wistream &din = std::wcin;
std::wostream &dout = std::wcout;
struct term;
typedef map<resid, term*> subs;
typedef vector<term*> termset;
//string lower ( const string& s_ ) { string s = s_; std::transform ( s.begin(), s.end(), s.begin(), ::towlower ); return s; }
string wstrim(string s) { trim(s); return s; }
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
string format(const subs& s);
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }

class wruntime_error : public std::exception {
	string msg;
public:
	wruntime_error(string s) : msg(s){}
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT { return std::string(msg.begin(), msg.end()).c_str(); }
	virtual ~wruntime_error() _GLIBCXX_USE_NOEXCEPT {}
};

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

class bidict {
	map<resid, string> ip;
	map<string, resid> pi;
public:
	void init() {
	}

	resid set ( string v ) {
		if (!v.size()) throw std::runtime_error("bidict::set called with a node containing null value");
		map<string, resid>::iterator it = pi.find ( v );
		if ( it ) return it->second;
		resid k = pi.size() + 1;
		if ( v[0] == L'?' ) k = -k;
//		pi.set(v, k), ip.set(k, v);
		pi[v] = k, ip[k] = v;
		return k;
	}

	string operator[] ( resid k ) { return ip[k]; }
	resid operator[] ( string v ) { return set(v); }
} dict;

const resid implies = dict.set(L"=>");
const resid Dot = dict.set(L".");

term* mkterm();
term* mkterm(termset& kb, termset& query);
term* mkterm(resid p);
term* mkterm(resid p, const termset& args);


enum Provenance {pbody, phead};

typedef std::pair<resid, Provenance> ruleres;
typedef std::pair<term*,Provenance> ruleterm;
typedef std::map<ruleres,ruleterm> DNA;
/*
typedef term* (*fevaluate)(term&, const subs&);
typedef bool (*funify)(term& s, const subs& ssub, term& d, subs& dsub);
typedef bool (*funify_ep)(term& s, const subs& ssub, term& d, const subs& dsub);
*/
struct term {
	term() : p(0) { throw 0; }
	term(termset& kb, termset& query) : p(0) { addbody(query); trymatch(kb); }
	term(resid _p, const termset& _args = termset()) : p(_p), args(_args) {
		TRACE(if (!p) throw 0);
	}

//i think i got it
	const resid p;
	const termset args;
	struct body_t {
		friend struct term;
		term* t;
		bool state;
		body_t(term* _t) : t(_t), state(false) {}
		termset matches;
		std::map<term*,map<ruleterm,vector<ruleres>>> dna;		
	};
	typedef vector<body_t*> bvec;
	bvec body;
	term* val;

	// add term(s) to body
	term* addbody(const termset& t) { for (termset::iterator it = t.begin(); it != t.end(); ++it) addbody(*it); return this; }
	term* addbody(term* t) {
		TRACE(dout << "added body " << format(t) << " to " << format(this) << std::endl);
		body.push_back(new body_t(t));
		return this;
	}

	// indexer: given rule's heads, see which can match to each body
	void trymatch(termset& heads) {
		for (bvec::iterator b = body.begin(); b != body.end(); ++b)
			for (termset::iterator it = heads.begin(); it != heads.end(); ++it)
				trymatch(**b, *it);
	}
	static term* evaluate(term* _t, subs& ss) {
		static term *v, *r;
		if (!_t) throw 0;
		term& t = *_t;
		if (t.p < 0)
			return (v = ss[t.p]) ? r = v->evaluate(v, ss) : 0;
		if (t.args.empty())
			return &t;
		termset ts;
		for (term* a : t.args) {
			if ((v = a->evaluate(a, ss))) ts.push_back(v);
			else ts.push_back(mkterm(a->p));
		}
		return mkterm(t.p, ts);
		TRACE(dout<<"evaluate " << format(&t) << " under " << format(ss) << " returned " << format(r) << std::endl);
		return r;
	};
/*
ugh..this would be so much more straightforward in univar lol
lets copy it into a different version if we wanna rework to univars,
a) that'll give us more leeway to rework shit, b) we should probably finish the
current version in case ohad's picky about it

this is just defficient, or seems so
anyway i wouldnt rework it to univars as much as just write the code into univar.cpp
ah, well, then even moreso on b) lol, not that working this into univar would be a bad idea
doesnt need to be worked into anything, just needs to print the output

well, let's switch to univar then
*/

	bool unify(subs& ssub, term& d, subs& dsub) {
		term* v;
		if (p < 0) return ((v=ssub[p]) && (v = v->evaluate(v, ssub))) ? v->unify(ssub, d, dsub) : true;
		if (d.p < 0) {
			if ((v = dsub[d.p]) && (v = d.evaluate(&d, dsub))) return unify(ssub, *v, dsub);
			dsub.set(d.p, v = evaluate(this, ssub));
//			TRACE(dout << "new sub: " << dict[d.p] << '\\' << format(v) << std::endl);
			return true;
		}
		size_t sz = args.size();
		if (p != d.p || sz != d.args.size()) return false;
		for (size_t n = 0; n < sz; ++n)
			if (!args[n]->unify(ssub, *d.args[n], dsub)) 
				return false;
		return true;
	}
	
	

	bool unify_ep(subs& ssub, term& d, subs& dsub) {
		static term* v;
		if (p < 0) return ((v=ssub[p]) && (v = v->evaluate(v, ssub))) ? v->unify_ep(ssub, d, dsub) : true;
		if (d.p < 0) return ((v=dsub[d.p]) && (v = v->evaluate(v, dsub))) ? unify_ep(ssub, *v, dsub) : true;
		size_t sz = args.size();
		if (p != d.p || sz != d.args.size()) return false;
		for (size_t n = 0; n < sz; ++n)
			if (!args[n]->unify_ep(ssub, *d.args[n], dsub)) 
				return false;
		return true;
	}
	void _unify(subs& ssub, termset& ts, sp_frame f, sp_frame& lastp);

private:
	// indexer: match a term (head) to a body's term by trying to unify them.
	// if they cannot unify without subs, then they will not be able to
	// unify also during inference.
	void trymatch(body_t& b, term* t) {
		//if (t == this) return;
		//b.addmatch(t, subs()); return; // unremark to disable indexing
		TRACE(dout << "trying to match " << format(b.t) << " with " << format(t) << std::endl);
		static subs d, empty;
		if (b.t->unify(empty, *t, d)) {
			TRACE(dout << "added match " << format(t) << " to " << format(b.t) << std::endl);
			b.matches.push_back(t);
		}
		d.clear();
	}
};


//ruleid won't work due to recursive rules ic 
//didnt we say the key should be a resid?
//key should be resid + Provenance
	//that doesn't really play well with find_root as we have it now though



term* new_all_vars_list(int count, Provenance p, DNA& dna)
{
	termset ts;
	static int name = 0;
	int i;
	for (i = 0; i < count; i++)
	{
		std::wstringstream ss;
		ss << "?new_var" << name++;
		term *tmp = mkterm(dict[ss.str()]);
		dna[ruleres(tmp->p,p)] = ruleterm(tmp,p);
		ts.push_back(tmp);
	}
	return mkterm(Dot, ts);
}



//mmm

ruleterm find_root(ruleterm t, DNA& dna){
	//do we ever deal with terms other than lists that have args and vars in them?
//ofc we do ?
//not sure if here but a body item or head is such a term
//ah yea, but not here, the only terms with args that we'll encounter here are lists
	assert(!t.first->args.size() || t.first->p == Dot);
	//nope, just vars, consts & lists, no triples/rules at this level
	/*ok so question is what is the root if there are lists
	(a b ?c) and (?a b c)
	would be a new list (a b c), i.e. the result of unifying all the lists
	in the set together
	*/
	
	//see we're returning a term here but the resid+Provenance loses all that info
	//so, maybe we should be sending find_root a termid+Provenance but still keying
	//the map with resid+Provenance
	//it's a little schizo but its pretty much the last thing left to do
	
	if(t.first->p > 0) return t;
	
	//we shouldn't even be getting lists beyond this point
	assert(!t.first->args.size());
	
	ruleres root(t.first->p, t.second);


	assert(!has(dna,root) || has(dna,root));

	if(!has(dna,root)){
		//dout << "New var: (" << t << ":" << t->p << ")" << format(t,false) << "\n";
		dna[root] = t;
		return t;
	}

	assert(has(dna, root));
	//this is just preparing the "next one"
	ruleterm rt = dna[root];

	//we don't want these asserts here, we want it to be able to be different
	//assert(root.first == dna[root].first->p);
	//assert(root.second == dna[root].second);

	ruleres rtmp = ruleres(rt.first->p,rt.second);
	//won't necessarily be there if it's a const or list
	//assert(has(dna, rtmp));

	while(rtmp.first < 0 && root != rtmp){
		root = rtmp;
		
		assert(has(dna, root));
		rt = dna[root];
		assert(rt.first);
		rtmp = ruleres(rt.first->p,rt.second);
	}

	//you sure we dont need it in the value?
	//hrm, well we don't need it in the ultimate return value
	//i guess we can repackage into a ruleterm
	//yea

	while(t != rt){
		ruleres tmp(t.first->p,t.second);
		ruleterm newt = dna[tmp];
		dna[tmp] = rt;
		t = newt;
	}

	//we can return either the ruleterm or the term*, the head/body distinction
	//is only necessary while we're inside find_root, this is fine though
	return rt;
}



bool makeDNA(term* b, term* h, DNA& dna){
	for(size_t n = 0; n < b->args.size(); n++){
		ruleterm br = find_root(ruleterm(b->args[n],pbody), dna);
		ruleterm hr = find_root(ruleterm(h->args[n],phead), dna);
		ruleres brr(br.first->p,br.second);
		ruleres hrr(hr.first->p,hr.second);
		
		//assert(!hr->val && !br->val);

		//var var		succeed
		//			make the body-item var the root
		if(br.first->p < 0 && hr.first->p < 0){
			dna[hrr] = br;
			continue;
		}

		
		//var const		succeed
		//			make the const the root
					
		//var list		succeed
		//			make the list the root
		if(br.first->p < 0 && hr.first->p > 0){
			if(!hr.first->args.size()){
				dna[brr] = hr;
				continue;
			}else{
				if(!dna[brr].first->args.size()){
					term* tmp = new_all_vars_list(hr.first->args.size(),pbody,dna);
					dna[brr] = ruleterm(tmp,pbody);
				}
				br = find_root(br,dna);
			}
		}
		
		//const var		succeed
		//			make the const the root
		//list var		succeed
		//			make the list the root
		assert(br.first && hr.first);
		if(br.first->p > 0 && hr.first->p < 0){
			//dna[hr] = br;
			if(!br.first->args.size()){
				dna[hrr] = br;
				continue;
			}else{
			//make an all-vars list of the same size and then
			//call makeDNA with br & the new list
			//reuse hr for the new list since we don't need the
			//var at this point
				if(!dna[hrr].first->args.size()){
					term* tmp = new_all_vars_list(br.first->args.size(),phead,dna);
					dna[hrr] = ruleterm(tmp,phead);
				}
				hr = find_root(hr,dna);
			}
		}
		
		//const const		succeed if const==const
		//			root is still the const, just
		//			merge the sets, otherwise fail
		
		//const list		fail
		
		//list const		fail
		
		//list list		succeed if list unifies with list
		//			the result of their unification
		//			becomes the root of the merged set

		if(!(br.first->p == br.first->p)) return false;
		if(!(br.first->args.size() == hr.first->args.size())) return false;
		if(!makeDNA(br.first,hr.first,dna)) return false;
	}

	return true;
}

void makeDNA(termset* rules){
	term** it = rules->begin();
	term** end = rules->end();
	for(; it != end; it++){
		term::body_t** bit = (*it)->body.begin();
		term::body_t** bend = (*it)->body.end();
		for(; bit != bend; bit++){
			term** hit = (*bit)->matches.begin();
			term** hend = (*bit)->matches.end();
			for(; hit != hend; hit++){
				DNA dna;
				//so, we'll want to makeDNA just like we're doing now,
				//but then go through the DNA structure and invert the
				//mapping so that it's <ruleterm,vector<ruleterm>>
				
				//<root, list-of-things-that-have-it-as-root>
				
				if(makeDNA((*bit)->t,(*hit),dna)){
					//when we invert the mapping, our values now
					//have to serve as keys, but they don't,
					//cause terms
					//but resid won't work because we need lists
					map<ruleterm,vector<ruleres>> newdna;
					for(auto x : dna){
						ruleterm tmp(mkterm(x.first.first),x.first.second);
						ruleterm root = find_root(tmp,dna);
						
						//if its a list we don't need to do
						//anything, cause all lists are different
						
						if(root.first->args.size())
						{
							assert(root.first->p == Dot);
					
							
							newdna[root].push_back(x.first);
						}
							
						else
						{
							bool found = false;
							for (auto kv: newdna)
							{
								if (kv.first.first->p == root.first->p && kv.first.second == root.second)
								{
									found = true;
									newdna[kv.first].push_back(x.first);
									break;
								}
							}
							if (!found){newdna[root].push_back(x.first);}
							
						}	
							
						//alright
						//hold tight
						//newdna[root].push_back(x.first);
					}
					(*bit)->dna[(*hit)] = newdna;
					//(*bit)->dna[(*hit)] = dna;
				}
			}
		}
	}
}
















const size_t tchunk = 8192, nch = tchunk / sizeof(term);
size_t bufpos = 0;
char* buf = (char*)malloc(tchunk);

#define MKTERM if (bufpos == nch) { buf = (char*)malloc(tchunk); bufpos = 0; } new (&((term*)buf)[bufpos])(term); return &((term*)buf)[bufpos++];
#define MKTERM1(x) if (bufpos == nch) { buf = (char*)malloc(tchunk); bufpos = 0; } new (&((term*)buf)[bufpos])(term)(x); return &((term*)buf)[bufpos++];
#define MKTERM2(x, y) if (bufpos == nch) { buf = (char*)malloc(tchunk); bufpos = 0; } new (&((term*)buf)[bufpos])(term)(x, y); return &((term*)buf)[bufpos++];

term* mkterm() { MKTERM }
term* mkterm(termset& kb, termset& query) { MKTERM2(kb, query); }
//	return new term(kb, query); }
term* mkterm(resid p, const termset& args) { MKTERM2(p, args); }
//	return new term(p, args); }
term* mkterm(resid p) { MKTERM1(p); }
//	return new term(p); }

#define EPARSE(x) throw wruntime_error(string(x) + string(s,0,48));
#define SKIPWS while (iswspace(*s)) ++s
#define RETIF(x) if (*s == x) return 0
#define RETIFN(x) if (*s != x) return 0
#define PROCEED1 while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')')
#define PROCEED PROCEED1 t[pos++] = *s++; t[pos] = 0; pos = 0
#define TILL(x) do { t[pos++] = *s++; } while (x)
class nqparser {
private: 
	wchar_t *t;
	const wchar_t *s;
	int pos;
public:
	nqparser() : t(new wchar_t[4096*4096]) {}
	~nqparser() { delete[] t; }

	bool readcurly(termset& ts) {
		while (iswspace(*s)) ++s;
		if (*s != L'{') return false;
		ts.clear();
		do { ++s; } while (iswspace(*s));
		if (*s == L'}') ++s;
		else ts = (*this)(s);
		return true;
	}

	term* readlist() {
		if (*s != L'(') return (term*)0;
		++s;
		termset items;
		term* pn;
		while (*s != L')') {
			SKIPWS;
			if (*s == L')') break;
			if (!(pn = readany(true)))
				EPARSE(L"couldn't read next list item: ");
			items.push_back(pn);
			SKIPWS;
//			pn = pn->o = mkterm(Dot);
			if (*s == L'.') while (iswspace(*s++));
			if (*s == L'}') EPARSE(L"expected { inside list: ");
		};
		do { ++s; } while (iswspace(*s));
		return mkterm(Dot, items);
	}

	term* readiri() {
		while (iswspace(*s)) ++s;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0; ++s;
			return mkterm(dict[t]);
		}
		if (*s == L'=' && *(s+1) == L'>') {
			++++s;
			return mkterm(implies);
		}
		PROCEED;
		string iri = wstrim(t);
		return mkterm(dict[iri]);
	}

	term* readbnode() { SKIPWS; if (*s != L'_' || *(s+1) != L':') return 0; PROCEED; return mkterm(dict[wstrim(t)]); }

	term* readvar() { SKIPWS; RETIFN(L'?'); PROCEED; return mkterm(dict[wstrim(t)]); }

	term* readlit() {
		SKIPWS; RETIFN(L'\"');
		++s;
		TILL(!(*(s-1) != L'\\' && *s == L'\"'));
		string dt, lang;
		++s;
		PROCEED1 {
			if (*s == L'^' && *++s == L'^') {
				if (*++s == L'<') {
					++s; while (*s != L'>') dt += *s++;
					++s; break;
				}
			} else if (*s == L'@') { while (!iswspace(*s)) lang += *s++; break; }
			else EPARSE(L"expected langtag or iri:");
		}
		t[pos] = 0; pos = 0;
		string t1 = t;
//boost::replace_all(t1, L"\\\\", L"\\");

		return mkterm(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
	}

	term* readany(bool lit = true) {
		term* pn;
		return ((pn = readbnode()) || (pn = readvar()) || (lit && (pn = readlit())) || (pn = readlist()) || (pn = readiri()) ) ? pn : 0;
	}

	void addhead(termset& ts, term* t) {
		if (!t) throw 0;
		if (!t->p) throw 0;
//		if (!t->s != !t->p) throw 0;
		ts.push_back(t);
	}

	termset operator()(const wchar_t* _s) {
		//typedef vector<std::pair<term*, termset> > preds_t;
		typedef map<term*, termset> preds_t;
		preds_t preds;
		s = _s;
//		string graph;
		term *subject, *pn;
		termset subjs, objs, heads;
		pos = 0;
//		auto pos1 = preds.back();
		termset dummy;
		preds_t::iterator pos1 = 0;

		while(*s) {//read facts & rules
			if (readcurly(subjs)) subject = 0;
			else if (!(subject = readany(false))) EPARSE(L"expected iri or bnode subject:"); // read subject
			do { // read predicates
				while (iswspace(*s) || *s == L';') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readiri())) { // read predicate
					preds.set(pn, termset());
					pos1 = preds.back();
				} else EPARSE(L"expected iri predicate:");
				do { // read objects
					while (iswspace(*s) || *s == L',') ++s;
					if (*s == L'.' || *s == L'}') break;
					if (readcurly(objs)) pos1->second = objs;
					else if ((pn = readany(true))) pos1->second.push_back(pn); // read object
					else EPARSE(L"expected iri or bnode or literal object:");
					SKIPWS;
				} while (*s == L','); // end objects
				SKIPWS;
			} while (*s == L';'); // end predicates
			if (*s != L'.' && *s != L'}' && *s) { // read graph
				if (!(pn = readbnode()) && !(pn = readiri()))
					EPARSE(L"expected iri or bnode graph:");
//				graph = dict[pn->p];
			} //else graph = ctx;
			SKIPWS; while (*s == '.') ++s; SKIPWS;
			if (*s == L')') EPARSE(L"expected ) outside list: ");
			if (subject)
				for (auto x = preds.begin(); x != preds.end(); ++x)
					for (termset::iterator o = x->second.begin(); o != x->second.end(); ++o) {
						termset ts;
						ts.push_back(subject);
						ts.push_back(*o);
						addhead(heads, mkterm(x->first->p, ts));
					}
			else for (termset::iterator o = objs.begin(); o != objs.end(); ++o) addhead(heads, (*o)->addbody(subjs));
			if (*s == L'}') { ++s; return heads; }
			preds.clear();
			subjs.clear();
			objs.clear();
		}
		return heads;
	}
};

termset readterms ( std::wistream& is) {
	string s, c;
	nqparser p;
	std::wstringstream ss;
	while (getline(is, s)) {
		trim(s);
		if (s[0] == '#') continue;
		if (startsWith(s, L"fin") && wstrim(s.c_str() + 3) == L".") break;
		ss << ' ' << s << ' ';
	}
	return p((wchar_t*)ss.str().c_str());
}

string formatlist(const term* t, bool in = false) {
	if (!t) return L"";
	if (t->p != Dot)
		throw 0;
	std::wstringstream ss;
	if (!in) ss << L'(';
	ss << format(t->args, true);// << L' ' << format(t->s) << L' ';
	if (!in) ss << L')';
	return ss.str();
}

string format(const term* t, bool body) {
	if (!t || !t->p) return L"";
	std::wstringstream ss;
	if (body && t->p == implies) {
		ss << L'{';
		for (term::bvec::iterator x = t->body.begin(); x != t->body.end(); ++x) ss << format((*x)->t) << L';';
		ss << L'}';
		ss << format(t, false);
	}
	else if (!t->p) return L"";
//	else if (t->p != Dot) {
		ss << dict[t->p];
		if (t->args.size()) {
			ss << L'(';
			for (term* y : t->args) ss << format(y) << L' ';
			ss << L") ";
		}
//	}
//	else return formatlist(t);
	return ss.str();
}

string format(const subs& s) {
	std::wstringstream ss;
	vector<mapelem<resid, term*> > v((const vector<mapelem<resid, term*> >&)s);
	for (size_t n = 0; n < s.size(); ++n)
		ss << dict[v[n].first] << L'\\' << format(v[n].second) << L' ';
	return ss.str();
}

string format(const ruleres rr){
	std::wstringstream ss;
	ss << dict[rr.first] << ":" << (rr.second == pbody?"b":"h");
	return ss.str();
}

string format(const ruleterm rt){
	std::wstringstream ss;
	ss << format(rt.first, false) << ":" << (rt.second == pbody?"b":"h");
	return ss.str();
}

#define IDENT for (int n = 0; n < dep; ++n) ss << L'\t'

string format(const termset& t, int dep) {
	std::wstringstream ss;
	for (termset::iterator _x = t.begin(); _x != t.end(); ++_x) {
		term* x = *_x;
		if (!x || !x->p) continue;
		IDENT;
		ss << format(x, true);
		if (x->body.size()) 
			ss << L" implied by: ";
		else
			ss <<  L" a fact.";
		ss << std::endl;
		for (term::bvec::iterator y = x->body.begin(); y != x->body.end(); ++y) {
			IDENT;
			ss << L"\t" << format((*y)->t, true) << L" matches to heads:" << std::endl;
			for (termset::iterator z = (*y)->matches.begin(); z != (*y)->matches.end(); ++z) {
				IDENT;
				ss << L"\t\t" << format(*z, true) << std::endl;
			}
		}
	}
	return ss.str();
}

typedef vector<mapelem<term*, subs> > ground;
typedef map<resid, vector<mapelem<term*, ground> > > evidence;

struct frame {
	term* rule;
	term::bvec::iterator b;
	sp_frame prev, creator, next;
	subs s;
	ground g() const { // calculate the ground
		if (!creator.p) return ground();
		ground r = creator.p->g();
		termset empty;
		frame& cp = *creator.p;
		typedef mapelem<term*, subs> elem;
		if (btterm)
			r.push_back(elem(btterm, subs()));
		else if (cp.b != cp.rule->body.end()) {
			if (!rule->body.size())
				r.push_back(elem(rule, subs()));
		} else if (cp.rule->body.size())
			r.push_back(elem(creator.p->rule, creator.p->s));
		return r;	
	}
	term* btterm;
	frame(sp_frame c, term* r, term::bvec::iterator* l, sp_frame p, const subs&  _s = subs())
		: rule(r), b(l ? *l : rule->body.begin()), prev(p), creator(c), s(_s), btterm(0) { }
	frame(sp_frame c, frame& p, const subs& _s) 
		: rule(p.rule), b(p.b), prev(p.prev), creator(c), s(_s), btterm(0) { }
};
size_t steps = 0;

sp_frame prove(sp_frame _p, sp_frame& lastp) {
	if (!lastp.p) lastp = _p;
	evidence e;
	while (_p.p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << std::endl);
		sp_frame ep = _p;
		frame& p = *_p.p;
		term* t = p.rule, *epr;
		if (t) { // check for euler path
			while ((ep = ep.p->prev).p) {
				epr = ep->rule;
				if (epr == p.rule && epr->unify_ep(ep->s, *t, p.s)) {
					_p = _p->next;
					t = 0;
					break;
				}
			}
			if (!t) { _p = p.next; continue; }
		}
		if (p.b != p.rule->body.end()) (*p.b)->t->_unify(p.s, (*p.b)->matches, _p, lastp);
		else if (!p.prev.p) {
			#ifndef NOTRACE
			term* _t; // push evidence
			for (term::bvec::iterator r = p.rule->body.begin(); r != p.rule->body.end(); ++r) {
				if (!(_t = ((*r)->t->evaluate((*r)->t, p.s)))) continue;
				e[_t->p].push_back(mapelem<term*, ground>(_t, p.g()));
				dout << "proved: " << format(_t) << std::endl;
			}
			#endif
		}
		else { // if body is done, go up the tree
			frame& ppr = *p.prev;
			sp_frame r(new frame(_p, ppr, ppr.s));
			p.rule->unify(p.s, *(*((r->b)++))->t, r->s);
			prove(r, lastp);
		}
		_p = p.next;
	}
//	dout << "steps: " << steps << std::endl;
	return sp_frame();
}

void term::_unify(subs& ssub, termset& ts, sp_frame f, sp_frame& lastp) {
	subs dsub;
	//for every applicable rule head
	//or is this like a pred?
	for (term* _d : ts)  {
		term& d = *_d;
		//oops how did a term with different arity get here?
		//i dont think we'd be getting different arity here
		if (args.size() != d.args.size()) continue;

		termset::iterator it = args.begin(), end = args.end(), dit = d.args.begin();
		while (it != end) {
			term& x = **it++;
			if (!x.unify(ssub, **dit++, dsub)) {
				--it;
				break;
			}
		}
		if (it == end) 
			lastp = lastp->next = sp_frame(new frame(f, _d, 0, f, dsub));
		dsub.clear();
	}
}

void printDNA(termset kb){
	int i = 1;
	for (auto it = kb.begin(); it != kb.end(); ++it){
		//Print Rule
		dout << i++ << ". " << format(*it,true) << "\n";
		int j = 1;
		for(auto bit = (*it)->body.begin(); bit != (*it)->body.end(); ++bit){
			//Print Body Item
			dout << "  " << j++ << ". " << format((*bit)->t,false) << "\n";
			int k = 1;
			//<term*,map<ruleterm,vector<ruleres>>>
			auto dit = (*bit)->dna.begin();
			auto e = (*bit)->dna.end();
			for (;dit != e; ++dit) {
				//Print Matching Rule
				dout << "    " << k++ << ". " << format(dit->first,false) << "\n";
				int l = 1;
				//<ruleterm,vector<ruleres>>
				auto eit = dit->second.begin();
				for (;eit != dit->second.end(); ++eit){
					//Print DNA
					/*
					dout	<< "      " << l++ << ". " <<
						format(eit->first) << "/" << 
						format(eit->second) << "\n";
					*/
					dout << "	" << l++ << ". " << format(eit->first);
					
					//ruleres		
					for(auto fit = eit->second.begin(); fit != eit->second.end(); ++fit){
						ruleres tmp(eit->first.first->p, eit->first.second);
						if(*fit != tmp){
							dout << " = " << format(*fit);
						}	
					}
					
					dout << "\n";
				}
			}
		}
	}
}

int main() {
	dict.init();
	termset kb = readterms(din);
	


	/*
	termset query = readterms(din);
	// create the query term and index it wrt the kb
	term* q = mkterm(kb, query);
	// now index the kb itself wrt itself
	*/


	for (termset::iterator it = kb.begin(); it != kb.end(); ++it)
		(*it)->trymatch(kb);


	makeDNA(&kb);

	printDNA(kb);
	
	/*
	kb.push_back(q);
	TRACE(dout << "kb:" << std::endl << format(kb) << std::endl);

	// create the initial frame with the query term as the frame's rule
	sp_frame p(new frame(sp_frame(), q, 0, sp_frame(), subs())), lastp;
	clock_t begin = clock(), end;
	prove(p, lastp); // the main loop
	end = clock();
	dout << "steps: " << steps << " elapsed: " << (1000. * double(end - begin) / CLOCKS_PER_SEC) << "ms" << std::endl;
	*/
	return 0;
}
