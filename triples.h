#include <iostream>
#include <algorithm>
#include <string>
#include "containers.h"

using std::function;
using std::wostream;
using std::wstring;
using std::endl;
using std::wcin;
#define FOR(x, y) for (int x = 0; x < (int)y; ++x)
#define isvar(x) ((x).type == L'?')
#define islist(x) ((x).type == L'.')
wostream& dout = std::wcout;

struct res { // iri, var, or list
	wchar_t type; // the only member used in runtime
	union {
		const res** args; // list items, last item must be null.
		const wchar_t *val;
	};
	res(const wchar_t *v) : type(*v), val(wcsdup(v)) { }
	res(vector<const res*> _args) : type(L'.'), args(new const res*[_args.size()+1]) {
		int n = 0;
		for (auto x : _args) args[n++] = x;
		args[n] = 0;
	}
	~res() { if (args && type == L'.') delete[] args; if (val) free((wchar_t*)val); }
};

struct triple {
	const res *r[3]; // spo
	triple(const res *s, const res *p, const res *o) { r[0] = s, r[1] = p, r[2] = o; }
	triple(const triple& t) : triple(t.r[0], t.r[1], t.r[2]) {}
};

typedef vector<const triple*> triples;

struct rule {
	const triple* head;
	triples body;
	rule(const triple* h, const triples& b = triples()) : head(h), body(b) {}
	rule(const rule& r) : head(r.head), body(r.body) {}
};
vector<rule> rules;

wostream& operator<<(wostream&, const res&);
wostream& operator<<(wostream&, const triple&t);
wostream& operator<<(wostream&, const struct vm&);
wostream& operator<<(wostream&, const rule&);

// uniquely create resources and triples
const res* mkres(const wstring& v) { 
	static map<wstring, const res*> r; 
	static map<wstring, const res*>::iterator i; 
	i = r.find(v);
	if (i) return i->second;
	return r.set(v, new res(v.c_str()));
}

const res* mkres(const vector<const res*>& v) { 
	static map<vector<const res*>, const res*> r; 
	static map<vector<const res*>, const res*>::iterator i; 
	return (i = r.find(v)) ? i->second : r.set(v, new res(v));
}

const triple* mktriple(const res *s, const res *p, const res *o) {
	static map<const res*, map<const res*, map<const res*, const triple*>>> spo;
	static map<const res*, map<const res*, map<const res*, const triple*>>>::iterator i;
	static map<const res*, map<const res*, const triple*>>::iterator j;
	static map<const res*, const triple*>::iterator k;
	static const triple* r;
	if (!(i = spo.find(s)))
		r = spo.set(s).set(p).set(o, new triple(s, p, o));
	else {
		if (!(j = i->second.find(p)))
				r = i->second[p][o] = new triple(s, p, o);
		else {
			if (!(k = j->second.find(o)))
				r = j->second[o] = new triple(s, p, o);
			else r = k->second;
	}	}
//	dout << "new triple: " << *r << endl;
	return r;
}

struct din_t {
	wchar_t ch;
	bool f = false;
	wchar_t peek() 	{ return f ? ch : ((f = true), (wcin >> ch), ch); }// due to wcin.peek() not working
	wchar_t get() 	{ return f ? ((f = false), ch) : ((wcin >> ch), ch); }
	bool good() 	{ return wcin.good(); }
	void skip() 	{ while (good() && iswspace(peek())) get(); }
	wstring getline(){ wstring s; f = false, std::getline(wcin, s); return s; }
	din_t() 	{ wcin >> std::noskipws; }

	wstring& trim(wstring& s) {
		static wstring::iterator i = s.begin();
		while (iswspace(*i)) s.erase(i), i = s.begin();
		size_t n = s.size();
		if (n) {
			while (iswspace(s[--n]));
			s = s.substr(0, ++n);
		}
		return s;
	}
	wstring edelims = L")}.";
	wstring till() { 
		skip();
		static wchar_t buf[4096];
		static size_t pos;
		pos = 0;
		while 	(good() &&
			edelims.find(peek()) == wstring::npos && 
			!iswspace(peek()))
			buf[pos++] = get();
		buf[pos] = 0;
//		const wchar_t *r = wcsdup(ws.str().c_str());
		return wcslen(buf) ? buf : 0;
	}
	const res* readlist() {
		get();
		vector<const res*> items;
		while (skip(), (good() && peek() != L')'))
			items.push_back(readany()), skip();
		return get(), skip(), mkres(items);
	}
	const res* readany() {
		if (skip(), !good()) return 0;
		if (peek() == L'(') return readlist();
		wstring s = till();
		return s.size() ? mkres(s) : 0;
	}
	const triple* readtriple() {
		const res *s, *p, *o;
		if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany())) return 0;
		const triple* r = mktriple(s, p, o);
		if (skip(), peek() == L'.') get(), skip();
		return r;
	}
	void readdoc() { // TODO: support fin notation too. also support prefixes and remarks.
		const triple *t;
		while (good()) {
			triples body;
			if (skip(), peek() == L'{') {
				get();
				while (good() && peek() != L'}') body.push_back(readtriple());
				get(), skip(), get(), get(), skip(), get(), skip(); // "} => {";
				if (peek() == L'}') rules.push_back(rule(0, body)), skip();
				else while (good() && peek() != L'}') rules.push_back(rule(readtriple(), body));
				get();
			} else if ((t = readtriple()))
				rules.push_back(rule(t));
			if (skip(), peek() == L'.')
				get(), skip();
}	}	} din;

// serializers
wostream& operator<<(wostream& os, const rule& r) {
	os << L'{';
	for (const triple* t : r.body) os << *t << ' ';
	return os << L"} => { ", (r.head ? os << *r.head : os << L""), os << " }.";
}
wostream& operator<<(wostream& os, const triple& t) {
	return os << *t.r[0] << ' ' << *t.r[1] << ' ' << *t.r[2] << L'.';
}
wostream& operator<<(wostream& os, const res& r) {
	if (!islist(r)) return os << r.val;
	os << L'(';
	const res **a = r.args;
	while (*a) os << (**a++) << L' ';
	return os << L')'; 
}
void print() { dout << "rules: " << rules.size() << endl; for (auto r : rules) dout << r << endl; } 
