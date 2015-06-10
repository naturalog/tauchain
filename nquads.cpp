#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

parse_nqline::parse_nqline() { t = new wchar_t[4096]; }
parse_nqline::~parse_nqline() { delete[] t; }
std::list<quad> parse_nqline::operator()(const wchar_t* s, string ctx/* = L"@default"*/) {
	std::list<quad> r;
	string graph;
	pnode subject, pn;
	typedef std::list<pnode> plist;
	std::list<std::pair<pnode, plist>> preds;
	std::list<std::tuple<pnode, pnode, pnode>> lists;
	std::function<pnode()> readiri;
	std::function<pnode()> readbnode;
	std::function<pnode()> readvar;
	std::function<pnode()> readlit;
	pnode rdffst = mkiri(pstr(L"rdf:first"));
	pnode rdfrst = mkiri(pstr(L"rdf:rest"));

	auto readlist = [&]() {
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
			if (!(pn = readiri()) && !(pn = readbnode()) && !(pn = readvar()) && !(pn = readlit()))
				throw wruntime_error(string(L"expected iri or bnode or list in list: ") + string(s,0,48));
			pnode cons = mkbnode(pstr(id()));
			lists.emplace_back(cons, rdffst, pn);
			++lpos;
			lists.emplace_back(cons, rdfrst, mkbnode(pstr(id())));
		}
		do { ++s; } while (iswspace(*s));
		return mkbnode(pstr(head));
	};
	uint pos = 0;

	auto _readiri = [&]() {
		while (iswspace(*s)) ++s;
		if (*s != L'<') return readlist();
		while (*++s != L'>') t[pos++] = *s;
		t[pos] = 0; pos = 0;
		++s;
		return mkiri(wstrim(t));
	};
	readiri = _readiri;

	auto _readbnode = [&]() {
		while (iswspace(*s)) ++s;
		if (*s != L'_') return pnode(0);
		while (!iswspace(*s)) t[pos++] = *s++;
		t[pos] = 0; pos = 0;
		return mkbnode(wstrim(t));
	};
	readbnode = _readbnode;
	auto _readvar = [&]() {
		while (iswspace(*s)) ++s;
		if (*s != L'?') return pnode(0);
		while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.') t[pos++] = *s++;
		t[pos] = 0; pos = 0;
		++s;
		return mkiri(wstrim(t));
	};
	readvar = _readvar;
	auto _readlit = [&]() {
		while (iswspace(*s)) ++s;
		if (*s != L'\"') return pnode(0);
		do { t[pos++] = *s++; } while (!(*(s-1) != L'\\' && *s == L'\"'));
		string dt, lang;
		++s;
		while (!iswspace(*s) && *s != L'.') {
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
	readlit = _readlit;

	while(*s) {
		if (!(subject = readiri()) && !(subject = readbnode()) && !(subject = readvar()))
			throw wruntime_error(string(L"expected iri or bnode subject:") + string(s,0,48));
		do {
			while (iswspace(*s) || *s == L';') ++s;
			if (*s == L'.') break;
			if ((pn = readiri()))
				preds.emplace_back(pn, plist());
			else if (*s == L'=' && *++s == L'>') {
				preds.emplace_back(mkiri(pimplication), plist());
				++s;
			}
			else throw wruntime_error(string(L"expected iri predicate:") + string(s,0,48));

			do {
				while (iswspace(*s) || *s == L',') ++s;
				if (*s == L'.') break;
				if ((pn = readiri()) || (pn = readbnode()) || (pn = readvar()) || (pn = readlit()))
					preds.back().second.push_back(pn);
				else throw wruntime_error(string(L"expected iri or bnode or literal object:") + string(s,0,48));
				while (iswspace(*s)) ++s;
			} while (*s == L',');
			while (iswspace(*s)) ++s;
		} while (*s == L';');
		if (*s != L'.' && *s) {
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
		while (*s == '.') ++s;
	}
	return r;
}
