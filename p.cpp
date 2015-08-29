#include "p.h"

std::wistream &din = std::wcin;
std::wostream &dout = std::wcout;
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }

subs ssub;
bidict dict;

const resid implies = dict.set(L"=>");
const resid Dot = dict.set(L".");

term::term() : p(0), szargs(0) { throw 0; }
term::term(termset& kb, termset& query) : p(0), szargs(0), body(query) { trymatch(kb); }
term::term(resid _p, const termset& _args) : p(_p), args(_args), szargs(args.size()),
		evaluate(p < 0 ? evvar : !szargs ? evnoargs : ev),
		evaluates(p < 0 ? evvars : !szargs ? evnoargss : evs),
		unify(p < 0 ? u1 : u3),
		unify_ep(p < 0 ? u2 : u4) {}

void term::trymatch(termset& heads) {
	subs d, e;
	for (termset::iterator _b = body.begin(); _b != body.end(); ++_b)
		for (termset::iterator it = heads.begin(); it != heads.end(); ++it) {
			term& b = **_b;
			ssub = e;
			if (b.unify(b, **it, d)) b.matches.push_back(*it);
			d.clear();
		}
}

term* evvar(term& t) {
	subs::vtype* v = ssub.find(t.p);
	return v ? v->second->evaluate(*v->second) : (term*)0; 
}

term* ev(term& t) {
	static term *v;
	termset ts;
	termset::coro c(t.args);
	while (c()) {
		term& b = **c.i;
		ts.push_back(((v = b.evaluate(b))) ? v : mkterm(b.p));
	}
	return mkterm(t.p, ts);
}

term* evvars(term& t, const subs& ss) {
	subs::vtype* v = ss.find(t.p);
	return v ? v->second->evaluates(*v->second, ss) : (term*)0; 
}

term* evs(term& t, const subs& ss) {
	static term *v;
	termset ts;
	termset::coro c(t.args);
	while (c()) {
		term& b = **c.i;
		ts.push_back(((v = b.evaluates(b, ss))) ? v : mkterm(b.p));
	}
	return mkterm(t.p, ts);
}

term* evnoargs(term& t) { return &t; }
term* evnoargss(term& t, const subs&) { return &t; }

#define FASTEVAL(x) (((x).p > 0 && !(x).szargs) ? (&x) : (x).evaluate(x))
#define FASTEVALS(x,s) (((x).p > 0 && !(x).szargs) ? (&x) : (x).evaluates(x,s))

bool u1(term& s, term& d, subs& dsub) {
	static subs::vtype *v;
	static term *e;
	if (!(v = ssub.find(s.p))) return true;
	term& vs = *v->second;
	return ((e = FASTEVAL(vs))) ? e->unify(*e, d, dsub) : true;
}

bool u2(term& s, term& d, const subs& dsub) { 
	static subs::vtype *v;
	static term *e;
	if (!(v = ssub.find(s.p))) return true;
	term& vs = *v->second;
	return ((e = FASTEVAL(vs))) ? e->unify_ep(*e, d, dsub) : true;
}

bool u3(term& s, term& d, subs& dsub) {
	if (d.p < 0) {
		subs::vtype* v = dsub.find(d.p);
		term* e = v ? FASTEVALS(*v->second, dsub) : 0;
		if (e) return s.unify(s, *e, dsub);
		dsub.set(d.p, FASTEVAL(s));
		return true;
	}
	if (s.p != d.p || s.szargs != d.args.size()) return false;
	const termset& ar = s.args;
	termset::iterator dit = d.args.begin();
	termset::coro c(ar);
	while (c())
		if (!(*c.i)->unify(**c.i, **dit++, dsub)) 
			return false;
	return true;
}

bool u4(term& s, term& d, const subs& dsub) {
	if (d.p < 0) {
		subs::vtype* v = dsub.find(d.p);
		term* e = v ? FASTEVALS(*v->second, dsub) : 0;
		return e ? s.unify_ep(s, *e, dsub) : true;
	}
	if (s.p != d.p || s.szargs != d.args.size()) return false;
	const termset& ar = s.args;
	termset::iterator dit = d.args.begin();
	termset::coro c(ar);
	while (c())
		if (!(*c.i)->unify_ep(**c.i, **dit++, dsub)) 
			return false;
	return true;
}

size_t bufpos = 0;
char* buf = (char*)malloc(tchunk);

#define MKTERM
#define MKTERM1(x)
#define MKTERM2(x, y) \
	if (bufpos == nch) { \
		buf = (char*)malloc(tchunk); \
		bufpos = 0; \
	} \
	new (&((term*)buf)[bufpos])(term)(x, y); \
	term* r = &((term*)buf)[bufpos]; \
	std::set<term*>::iterator it = terms.find(r); \
	if (it != terms.end()) return *it; \
	bufpos++; \
	return r; 

term* mkterm() {
	if (bufpos == nch) {
		buf = (char*)malloc(tchunk);
		bufpos = 0;
	}
	new (&((term*)buf)[bufpos])(term);
	term* r = &((term*)buf)[bufpos];
	std::set<term*>::iterator it = terms.find(r);
	if (it != terms.end()) return *it;
	bufpos++;
	terms.insert(r);
	return r;
}
term* mkterm(termset& kb, termset& query) { MKTERM2(kb, query); }
term* mkterm(resid p, const termset& args) { MKTERM2(p, args); }
term* mkterm(resid p) {
	if (bufpos == nch) {
		buf = (char*)malloc(tchunk);
		bufpos = 0;
	}
	new (&((term*)buf)[bufpos])(term)(p);
	term* r = &((term*)buf)[bufpos];
	std::set<term*>::iterator it = terms.find(r);
	if (it != terms.end()) return *it;
	bufpos++;
	terms.insert(r);
	return r;
}

#define EPARSE(x) dout << (string(x) + string(s,0,48)) << endl, throw 0
#define SKIPWS while (iswspace(*s)) ++s
#define RETIF(x) if (*s == x) return 0
#define RETIFN(x) if (*s != x) return 0
#define PROCEED1 while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')')
#define PROCEED PROCEED1 t[pos++] = *s++; t[pos] = 0; pos = 0
#define TILL(x) do { t[pos++] = *s++; } while (x)

nqparser::nqparser() : t(new wchar_t[4096*4096]) {}
nqparser::~nqparser() { delete[] t; }

bool nqparser::readcurly(termset& ts) {
	while (iswspace(*s)) ++s;
	if (*s != L'{') return false;
	ts.clear();
	do { ++s; } while (iswspace(*s));
	if (*s == L'}') ++s;
	else ts = (*this)(s);
	return true;
}

term* nqparser::readlist() {
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
		if (*s == L'.') while (iswspace(*s++));
		if (*s == L'}') EPARSE(L"expected { inside list: ");
	};
	do { ++s; } while (iswspace(*s));
	return mkterm(Dot, items);
}

term* nqparser::readiri() {
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

term* nqparser::readbnode() { SKIPWS; if (*s != L'_' || *(s+1) != L':') return 0; PROCEED; return mkterm(dict[wstrim(t)]); } 
term* nqparser::readvar() { SKIPWS; RETIFN(L'?'); PROCEED; return mkterm(dict[wstrim(t)]); }

term* nqparser::readlit() {
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
//boost:replace_all(t1, L"\\\\", L"\\");
	return mkterm(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
}

term* nqparser::readany(bool lit) {
	term* pn;
	return ((pn = readbnode()) || (pn = readvar()) || (lit && (pn = readlit())) || (pn = readlist()) || (pn = readiri()) ) ? pn : 0;
}

void nqparser::addhead(termset& ts, term* t) {
	if (!t) throw 0;
	if (!t->p) throw 0;
	ts.push_back(t);
}

termset nqparser::operator()(const wchar_t* _s) {
	typedef map<term*, termset, false> preds_t;
	preds_t preds;
	s = _s;
//	string graph;
	term *subject, *pn;
	termset subjs, objs, heads;
	pos = 0;
	preds_t::iterator pos1 = 0;

	while(*s) {
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
			} while (*s == L','); // end predicates
			SKIPWS;
		} while (*s == L';'); // end objects
		if (*s != L'.' && *s != L'}' && *s) { // read graph
			if (!(pn = readbnode()) && !(pn = readiri()))
				EPARSE(L"expected iri or bnode graph:");
//			graph = dict[pn->p];
		} //else graph = ctx;
		SKIPWS; while (*s == '.') ++s; SKIPWS;
		if (*s == L')') EPARSE(L"expected ) outside list: ");
		if (subject)
			for (preds_t::vtype* x = preds.begin(); x != preds.end(); ++x)
				for (termset::iterator o = x->second.begin(); o != x->second.end(); ++o) {
					termset ts;
					ts.push_back(subject);
					ts.push_back(*o);
					addhead(heads, mkterm(x->first->p, ts));
				}
		else for (termset::iterator o = objs.begin(); o != objs.end(); ++o) {
			(*o)->body = subjs;
			addhead(heads, *o);
		}
		if (*s == L'}') { ++s; return heads; }
		preds.clear();
		subjs.clear();
		objs.clear();
	}
	return heads;
}

termset nqparser::readterms(std::wistream& is) {
	string s, c;
	nqparser p;
	string ss, space = L" ";
	while (getline(is, s)) {
		trim(s);
		if (s[0] == '#') continue;
		if (startsWith(s, L"fin") && wstrim(s.c_str() + 3) == L".") break;
		ss += space + s + space;
	}
	return p((wchar_t*)ss.c_str());
}

string format(const term* t, bool body) {
	if (!t || !t->p) return L"";
	string ss;
	if (body && t->p == implies) {
		ss += L'{';
		for (termset::iterator x = t->body.begin(); x != t->body.end(); ++x) ss += format(*x) + L';';
		ss += L'}';
		ss += format(t, false);
	}
	else if (!t->p) return L"";
	ss += dict[t->p];
	if (t->args.size()) {
		ss += L'(';
		for (term** y = t->args.begin(); y != t->args.end(); ++y) ss += format(*y) += L' ';
		ss += L") ";
	}
	return ss;
}

string format(const subs& s) {
	string ss;
	vector<mapelem<resid, term*>, true > v((const vector<mapelem<resid, term*>, true >&)s);
	for (size_t n = 0; n < s.size(); ++n)
		ss += dict[v[n].first] += L'\\' + format(v[n].second) += L' ';
	return ss;
}

#define IDENT for (int n = 0; n < dep; ++n) ss += L'\t'

string format(const termset& t, int dep) {
	string ss;
	for (termset::iterator _x = t.begin(); _x != t.end(); ++_x) {
		term* x = *_x;
		if (!x || !x->p) continue;
		IDENT;
		ss += format(x, true);
		if (x->body.size()) 
			ss += L" implied by: ";
		else
			ss +=  L" a fact.";
		ss += endl;
		for (termset::iterator y = x->body.begin(); y != x->body.end(); ++y) {
			IDENT;
			((ss += L"\t") += format(*y, true) += L" matches to heads:") += endl;
			for (termset::iterator z = (*y)->matches.begin(); z != (*y)->matches.end(); ++z) {
				IDENT;
				ss += L"\t\t" + format(*z, true) + endl;
			}
		}
	}
	return ss;
}

ground frame::g() const { 
	if (!creator) return ground();
	ground r = creator->g();
	termset empty;
	frame& cp = *creator;
	typedef mapelem<term*, subs> elem;
	if (cp.b != cp.rule->body.end()) {
		if (!rule->body.size())
			r.push_back(elem(rule, subs()));
	} else if (cp.rule->body.size())
		r.push_back(elem(creator->rule, creator->s));
	return r;	
}
frame::frame(pframe c, term* r, termset::iterator l, pframe p, const subs& _s)
		: rule(r), b(l ? l : rule->body.begin()), prev(p), creator(c), next(0), s(_s), ref(0) { if(c)++c->ref;if(p)++p->ref; }
frame::frame(pframe c, term* r, termset::iterator l, pframe p)
		: rule(r), b(l ? l : rule->body.begin()), prev(p), creator(c), next(0), ref(0) { if(c)++c->ref;if(p)++p->ref; }
frame::frame(pframe c, frame& p, const subs& _s) 
		: rule(p.rule), b(p.b), prev(p.prev), creator(c), next(0), s(_s), ref(0) { if(c)++c->ref; if(prev)++prev->ref; }
void frame::decref() { if (ref--) return; if(creator)creator->decref();if(prev)prev->decref(); delete this; }

size_t steps = 0;

void prove(pframe _p, pframe& lastp) {
	if (!lastp) lastp = _p;
	evidence e;
//	subs dsub;
//	term **dit;
//	bool f;
	while (_p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << endl);
		pframe ep = _p;
		frame& p = *_p;
		term* t = p.rule, *epr;
		// check for euler path
		ssub = p.s;
		while ((ep = ep->prev)) {
			if ((epr = ep->rule) == p.rule && t->unify_ep(*t, *epr, ep->s)) {
				t = 0;
				break;
			}
		}
		if (!t);
		else if (p.b != p.rule->body.end()) {
			term::coro m(**p.b);
			while (m())
				lastp = lastp->next = pframe(new frame(_p, m.i, 0, _p, m.dsub));
		}
		else if (p.prev) { // if body is done, go up the tree
			frame& ppr = *p.prev;
			pframe r(new frame(_p, ppr, ppr.s));
			p.rule->unify(*p.rule, **r->b++, r->s);
			prove(r, lastp);
		}
		#ifndef NOTRACE
		else {
			term* _t; // push evidence
			for (termset::iterator r = p.rule->body.begin(); r != p.rule->body.end(); ++r) {
				if (!(_t = ((*r)->evaluate(**r)))) continue;
				e[_t->p].push_back(mapelem<term*, ground>(_t, p.g()));
				dout << "proved: " << format(_t) << endl;
			}
		}
		#endif
		pframe pf = _p; _p = p.next; pf->decref();
	}
}

int main() {
	termset kb = nqparser::readterms(din);
	termset query = nqparser::readterms(din);

	// create the query term and index it wrt the kb
	term* q = mkterm(kb, query);
	// now index the kb itself wrt itself
	for (termset::iterator it = kb.begin(); it != kb.end(); ++it)
		(*it)->trymatch(kb);
	kb.push_back(q);
	TRACE(dout << "kb:" << endl << format(kb) << endl);

	// create the initial frame with the query term as the frame's rule
	pframe p(new frame(pframe(), q, 0, pframe(), subs())), lp = 0;
	clock_t begin = clock(), end;
	prove(p, lp); // the main loop
	end = clock();
	dout << "steps: " << steps << " elapsed: " << (1000. * double(end - begin) / CLOCKS_PER_SEC) << "ms" << endl;

	return 0;
}
