#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
#include <boost/algorithm/string.hpp>
#include "jsonld.h"
using namespace boost::algorithm;

parse_nqline::parse_nqline(const wchar_t* _s) : t(new wchar_t[4096]), s(_s), rdffst(mkiri(pstr(L"rdf:first"))), rdfrst(mkiri(pstr(L"rdf:rest"))) {}
parse_nqline::~parse_nqline() { delete[] t; }


pnode parse_nqline::readcurly() {
	while (iswspace(*s)) ++s;
	if (*s != L'{') return (pnode)0;
	++s;
	while (iswspace(*s)) ++s;
	auto r = jsonld_api::gen_bnode_id();
	auto t = (*this)(*r);
	return mkbnode(r);
};

pnode parse_nqline::readlist() {
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
		lists.emplace_back(cons, rdfrst, mkbnode(pstr(id())));
	}
	do { ++s; } while (iswspace(*s));
	return mkbnode(pstr(head));
};

pnode parse_nqline::readiri() {
	while (iswspace(*s)) ++s;
	if (*s != L'<') return (pnode)0;
	while (*++s != L'>') t[pos++] = *s;
	t[pos] = 0; pos = 0;
	++s;
	return mkiri(wstrim(t));
};

pnode parse_nqline::readbnode() {
	while (iswspace(*s)) ++s;
	if (*s != L'_') return pnode(0);
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	return mkbnode(wstrim(t));
};

pnode parse_nqline::readvar() {
	while (iswspace(*s)) ++s;
	if (*s != L'?') return pnode(0);
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	++s;
	return mkiri(wstrim(t));
};

pnode parse_nqline::readlit() {
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

pnode parse_nqline::readany(bool lit){
	pnode pn;
	if (!(pn = readbnode()) && !(pn = readvar()) && (!lit || !(pn = readlit())) && !(pn = readiri()) && !(pn = readlist()) && !(pn = readcurly())   )
		return (pnode)0;
	return pn;
};


std::list<quad> parse_nqline::operator()(string ctx/* = L"@default"*/) {
	string graph;
	pnode subject, pn;
	std::list<std::pair<pnode, plist>> preds;
	pos = 0;

	while(*s) {
		if (!(subject = readany(false)))
			throw wruntime_error(string(L"expected iri or bnode subject:") + string(s,0,48));
		do {
			while (iswspace(*s) || *s == L';') ++s;
			if (*s == L'.' || *s == L'}') break;
			if ((pn = readiri()))
				preds.emplace_back(pn, plist());
			else if (*s == L'=' && *++s == L'>') {
				preds.emplace_back(mkiri(pimplication), plist());
				++s;
			}
			else throw wruntime_error(string(L"expected iri predicate:") + string(s,0,48));

			do {
				while (iswspace(*s) || *s == L',') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readany(true)))
					preds.back().second.push_back(pn);
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
		for (auto d : lists) {
			quad q(std::get<0>(d), std::get<1>(d), std::get<2>(d), L"@default"/*graph*/);
			dout << q.tostring() << std::endl;
			r.emplace_back(q);
		}
		for (auto x : preds) for (pnode object : x.second) {
			quad q(subject, x.first, object, graph);
			dout << q.tostring() << std::endl;
			r.emplace_back(q);
		}
		lists.clear();
		preds.clear();
		while (iswspace(*s)) ++s;
		while (*s == '.') ++s;
		while (iswspace(*s)) ++s;
		if (*s == L'}') { ++s; return r; }
	}
	return r;
}
