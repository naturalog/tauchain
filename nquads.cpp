#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
#include <boost/algorithm/string.hpp>
#include "jsonld.h"
#include "misc.h"
using namespace boost::algorithm;

nqparser::nqparser() : t(new wchar_t[4096]) {
	if (rnil) return;
	rdffst = mkiri(pstr(L"rdf:first")); 
	rdfrst = mkiri(pstr(L"rdf:rest"));
	rnil = mkiri(pstr(L"rdf:nil"));
}
nqparser::~nqparser() { delete[] t; }
pnode nqparser::rdfrst = 0;
pnode nqparser::rdffst = 0;
pnode nqparser::rnil = 0;

pnode nqparser::readcurly() {
	setproc(L"readcurly");
	while (iswspace(*s)) ++s;
	if (*s != L'{') return (pnode)0;
	++s;
	while (iswspace(*s)) ++s;
	auto r = jsonld_api::gen_bnode_id();
	auto t = (*this)(s, *r);
	return mkbnode(r);
};

pnode nqparser::readlist() {
	setproc(L"readlist");
	if (*s != L'(') return (pnode)0;
	static int lastid = 0;
	int lpos = 0, curid = lastid++;
	auto id = [&]() { std::wstringstream ss; ss << L"_:list" << curid << '.' << lpos; return ss.str(); };
	const string head = id();
	pnode pn;
	++s;
	while (*s != L')') {
		while (iswspace(*s)) ++s;
		if (*s == L')') break;
		if (!(pn = readany(true)))
			throw wruntime_error(string(L"expected iri or bnode or list in list: ") + string(s,0,48));
		pnode cons = mkbnode(pstr(id()));
		lists.emplace_back(cons, rdffst, pn);
		++lpos;
		while (iswspace(*s)) ++s;
		if (*s == L')') lists.emplace_back(cons, rdfrst, rnil);
		else lists.emplace_back(cons, rdfrst, mkbnode(pstr(id())));
		if (*s == L'.') while (iswspace(*s++));
	}
	do { ++s; } while (iswspace(*s));
	return mkbnode(pstr(head));
};

pnode nqparser::readiri() {
	setproc(L"readiri");
	while (iswspace(*s)) ++s;
	if (*s == L'<') {
		while (*++s != L'>') t[pos++] = *s;
		t[pos] = 0; pos = 0;
		++s;
		return mkiri(wstrim(t));
	}
	while (!iswspace(*s) && *s != L'.') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	++s;
	pstring iri = wstrim(t);
	if (*iri == L"=>") return mkiri(pimplication);
	auto i = iri->find(L':');
	if (i == string::npos) return mkiri(iri);//throw wruntime_error(string(L"expected ':' in iri: ") + string(s,0,48));
	string p = iri->substr(0, ++i);
	TRACE(dout<<"extracted prefix \"" << p <<L'\"'<< endl);
	auto it = prefixes.find(p);
	if (it != prefixes.end()) {
		TRACE(dout<<"prefix: " << p << " subst: " << *it->second->value<<endl);
		iri = pstr(*it->second->value + iri->substr(i));
	} //else if (p == L":") iri = pstr(L"");
	return mkiri(iri);
};

pnode nqparser::readbnode() {
	setproc(L"readbnode");
	while (iswspace(*s)) ++s;
	if (*s != L'_') return pnode(0);
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	return mkbnode(wstrim(t));
};

void nqparser::readprefix() {
	setproc(L"readprefix");
	while (iswspace(*s)) ++s;
	if (*s != L'@') return;
	if (memcmp(s, L"@prefix ", 8*sizeof(*s)))
			throw wruntime_error(string(L"@prefix expected: ") + string(s,0,48));
	s += 8;
	while (*s != L':') t[pos++] = *s++;
	t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	pstring tt = wstrim(t);
	TRACE(dout<<"reading prefix: \"" << *tt<<L'\"' << endl);
	prefixes[*tt] = readiri();
	while (*s != '.') ++s;
	++s;
};

pnode nqparser::readvar() {
	setproc(L"readvar");
	while (iswspace(*s)) ++s;
	if (*s != L'?') return pnode(0);
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	++s;
	return mkiri(wstrim(t));
};

pnode nqparser::readlit() {
	setproc(L"readlit");
	while (iswspace(*s)) ++s;
	if (*s != L'\"') return pnode(0);
	++s;
	do { t[pos++] = *s++; } while (!(*(s-1) != L'\\' && *s == L'\"'));
	string dt, lang;
	++s;
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}') {
		if (*s == L'^' && *++s == L'^') {
			if (*++s == L'<')  {
				++s;
				while (*s != L'>') dt += *s++;
				++s;
				break;
			}
		} else if (*s == L'@') { 
			while (!iswspace(*s)) lang += *s++;
			break;
		}
		else throw wruntime_error(string(L"expected langtag or iri:") + string(s,0,48));
	}
	t[pos] = 0; pos = 0;
	return mkliteral(wstrim(t), pstrtrim(dt), pstrtrim(lang));
};

pnode nqparser::readany(bool lit){
	pnode pn;
	readprefix();
	if (!(pn = readbnode()) && !(pn = readvar()) && (!lit || !(pn = readlit())) && !(pn = readlist()) && !(pn = readcurly()) && !(pn = readiri()) )
		return (pnode)0;
	return pn;
};


std::list<quad> nqparser::operator()(const wchar_t* _s, string ctx/* = L"@default"*/) {
	std::list<std::pair<pnode, plist>> preds;
	s = _s;
	string graph;
	pnode subject, pn;
	pos = 0;
	auto pos1 = preds.rbegin();

	while(*s) {
		if (!(subject = readany(false)))
			throw wruntime_error(string(L"expected iri or bnode subject:") + string(s,0,48));
		do {
			while (iswspace(*s) || *s == L';') ++s;
			if (*s == L'.' || *s == L'}') break;
			if ((pn = readiri())) {
				preds.emplace_back(pn, plist());
				pos1 = preds.rbegin();
			}
//			else if (*s == L'=' && *++s == L'>') {
//				preds.emplace_back(mkiri(pimplication), plist());
//				++s;
//			}
			else throw wruntime_error(string(L"expected iri predicate:") + string(s,0,48));
//			auto pos2 = pos1->second.rbegin();
			do {
				while (iswspace(*s) || *s == L',') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readany(true))) {
					pos1->/*preds.back().*/second.push_back(pn);
//					pos2 = pos1->second.rbegin();
				}
				else throw wruntime_error(string(L"expected iri or bnode or literal object:") + string(s,0,48));
				while (iswspace(*s)) ++s;
			} while (*s == L',');
			while (iswspace(*s)) ++s;
		} while (*s == L';');
		if (*s != L'.' && *s != L'}' && *s) {
			if (*s == L'<')
				while (*++s != L'>') t[pos++] = *s;
			else if (*s == L'_')
				while (!iswspace(*s)) t[pos++] = *s++;
			else throw wruntime_error(string(L"expected iri or bnode graph:") + string(s,0,48));
			t[pos] = 0; pos = 0;
			trim(graph = t);
			++s;
		} else
			graph = ctx;
//		auto l1 = lists;
//		auto p1 = preds;
		for (auto d : lists) {
//			if (std::get<0>(d) &&std::get<1>(d) && std::get<2>(d))
				r.emplace_back(std::get<0>(d), std::get<1>(d), std::get<2>(d), L"@default"/*graph*/);
//			else l1.push_back(d);
		}
		for (auto x : preds)
			for (pnode object : x.second)
//				if (object) 
				r.emplace_back(subject, x.first, object, graph);
//				else p1.push_back(x);
		lists.clear();
		preds.clear();
		while (iswspace(*s)) ++s;
		while (*s == '.') ++s;
		while (iswspace(*s)) ++s;
		if (*s == L'}') { ++s; return r; }
	}
	return r;
}
