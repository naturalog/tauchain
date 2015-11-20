#ifndef NOPARSER
#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
#include <boost/algorithm/string.hpp>
#include "jsonld.h"
#include "misc.h"
using namespace boost::algorithm;

namespace old{

nqparser::nqparser() : t(new wchar_t[4096*4096]) { }
nqparser::~nqparser() { delete[] t; }

pnode nqparser::readcurly() {
	setproc(L"readcurly");
	while (iswspace(*s)) ++s;
	if (*s != L'{') return (pnode)0;
	++s;
	while (iswspace(*s)) ++s;
	auto r = gen_bnode_id();
	if (*s == L'}') { ++s; return mkbnode(r); }
	auto t = (*this)(s, *r);
	return mkbnode(r);
}

pnode nqparser::readlist() {
	setproc(L"readlist");
	if (*s != L'(') return (pnode)0;
	static int lastid = 0;
	int lpos = 0, curid = lastid++;
	auto id = [&]() { std::wstringstream ss; ss << L"_:list" << curid << '.' << lpos; return ss.str(); };
	const string head = id();
	pnode pn;
	++s;
	while (iswspace(*s)) ++s;
	if (*s == L')') { ++s; return mkiri(RDF_NIL); }
	do {
		while (iswspace(*s)) ++s;
		if (*s == L')') break;
		if (!(pn = readany(true)))
			throw wruntime_error(string(L"expected iri or bnode or list in list: ") + string(s,0,48));
		pnode cons = mkbnode(pstr(id()));
		lists.emplace_back(cons, mkiri(RDF_FIRST), pn);
		qlists[head].push_back(pn);
		++lpos;
		while (iswspace(*s)) ++s;
		if (*s == L')') lists.emplace_back(cons, mkiri(RDF_REST), mkiri(RDF_NIL));
		else lists.emplace_back(cons, mkiri(RDF_REST), mkbnode(pstr(id())));
		if (*s == L'.') while (iswspace(*s++));
		if (*s == L'}') throw wruntime_error(string(L"expected { inside list: ") + string(s,0,48));
	}
	while (*s != L')');
	do { ++s; } while (iswspace(*s));
	return mkbnode(pstr(head));
}

pnode nqparser::readiri() {
	setproc(L"readiri");
	while (iswspace(*s)) ++s;
	if (*s == 0)
		throw std::runtime_error("iri expected");
	if (*s == L'<') {
		while (*++s != L'>') t[pos++] = *s;
		t[pos] = 0; pos = 0;
		++s;
		return mkiri(wstrim(t));
	}
	if (*s == L'=' && *(s+1) == L'>') { ++++s; return mkiri(pimplication); }
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	pstring iri = wstrim(t);
	if (lower(*iri) == L"true")
		return mkliteral(pstr(L"true"), XSD_BOOLEAN, 0);
	if (lower(*iri) == L"false")
		return mkliteral(pstr(L"false"), XSD_BOOLEAN, 0);
	if (std::atoi(ws(*iri).c_str()))
		return mkliteral(iri, XSD_INTEGER, 0);
	if (std::atof(ws(*iri).c_str()))
		return mkliteral(iri, XSD_DOUBLE, 0);
	if (*iri == L"0") return mkliteral(iri, XSD_INTEGER, 0);
	auto i = iri->find(L':');
	if (i == string::npos) return mkiri(iri);
	string p = iri->substr(0, ++i);
//	TRACE(dout<<"extracted prefix \"" << p <<L'\"'<< endl);
	auto it = prefixes.find(p);
	if (it != prefixes.end()) {
//		TRACE(dout<<"prefix: " << p << " subst: " << *it->second->value<<endl);
		iri = pstr(*it->second->value + iri->substr(i));
	}
	return mkiri(iri);
}

pnode nqparser::readbnode() {
	setproc(L"readbnode");
	while (iswspace(*s)) ++s;
	if (*s != L'_' || *(s+1) != L':') return pnode(0);
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	return mkbnode(wstrim(t));
}

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
}

pnode nqparser::readvar() {
	setproc(L"readvar");
	while (iswspace(*s)) ++s;
	if (*s != L'?') return pnode(0);
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	return mkiri(wstrim(t));
}

pnode nqparser::readlit() {
	setproc(L"readlit");
	while (iswspace(*s)) ++s;
	if (*s != L'\"') return pnode(0);
	++s;
	do { t[pos++] = *s++; } while (!(*(s-1) != L'\\' && *s == L'\"'));
	string dt, lang;
	++s;
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') {
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
	string t1 = t;
	boost::replace_all(t1, L"\\\\", L"\\");
	return mkliteral(wstrim(t1), pstrtrim(dt), pstrtrim(lang));
}

pnode nqparser::readany(bool lit){
	pnode pn;
	readprefix();
	if (!(pn = readbnode()) && !(pn = readvar()) && (!lit || !(pn = readlit())) && !(pn = readlist()) && !(pn = readcurly()) && !(pn = readiri()) )
		return (pnode)0;
	return pn;
}


int nqparser::nq_readfile(std::wistream& is, std::wstringstream& ss){
	int fins=0;
        string s; //get lines from is into s.

        //Make a function out of this
        //Iterate over lines coming from is, placing them into s, and
	//accumulating these strings into ss.
        while (getline(is, s)) {
                //dout << "line:\"" << s << "\"" << std::endl;

                //Trim white-space on the line.
                //(in what specific manner?)
                trim(s);

                //If it's an empty line or a comment then continue.
                if (!s.size() || s[0] == '#') continue;

                //Check to see if the line is "fin." or some variation where
                //white-space separates "fin" from "." (...?), and if so
                //break.
                if (startsWith(s, L"fin") && *wstrim(s.c_str() + 3) == L"."){
                        fins++;
                        break;
                }
//              dout << s << endl;

                //Otherwise, add this line into ss. 
                //(more trailing white-space).
                ss << ' ' << s<< ' ';
	}

	return fins;	
}

int nq_to_qdb(qdb& kb, std::wistream& is){

	std::wstringstream ss; //accumulate processed lines into ss
	//Read an nq file in is into the wstringstream ss.
	int fins = nq_readfile(is,ss);


//std::pair<std::list<quad>, std::map<string, std::list<pnode>>> nqparser::operator()(const wchar_t* _s, string ctx/* = L"@default"*/) {
	
	const wchar_t* _s = (wchar_t*)ss.str().c_str();
	string ctx;
	std::list<std::pair<pnode, plist>> preds;
	std::pair<std::list<quad>,std::map<string, std::list<pnode>>> rr;
	s = _s;
	if (!s || !*s) rr = {{},{}};
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
			if ((pn = readiri()) || (pn = readcurly())) {
				preds.emplace_back(pn, plist());
				pos1 = preds.rbegin();
			}
			else throw wruntime_error(string(L"expected iri predicate:") + string(s,0,48));
			do {
				while (iswspace(*s) || *s == L',') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readany(true))) {
					pos1->second.push_back(pn);
				}
				else throw wruntime_error(string(L"expected iri or bnode or literal object:") + string(s,0,48));
				while (iswspace(*s)) ++s;
			} while (*s == L',');
			while (iswspace(*s)) ++s;
		} while (*s == L';');
		if (*s != L'.' && *s != L'}' && *s) {
			if (!(pn = readbnode()) && !(pn = readiri()))
				throw wruntime_error(string(L"expected iri or bnode graph:") + string(s,0,48));
			graph = *pn->value;
		} else
			graph = ctx;
		for (auto d : lists)
				r.emplace_back(std::get<0>(d), std::get<1>(d), std::get<2>(d), graph);
		for (auto x : preds)
			for (pnode object : x.second)
				r.emplace_back(subject, x.first, object, graph);
		lists.clear();
		preds.clear();
		while (iswspace(*s)) ++s;
		while (*s == '.') ++s;
		while (iswspace(*s)) ++s;
		if (*s == L'}') { ++s; rr = { r, qlists }; }
		if (*s == L')') throw wruntime_error(string(L"expected ) outside list: ") + string(s,0,48));
	}
	rr = { r, qlists };


        kb.second = rr.second;
        for (quad q : r/.first) {
                string c = *q.graph->value;
                if (kb.first.find(c) == kb.first.end()) kb.first[c] = make_shared<qlist>();
                kb.first[c]->push_back(make_shared<quad>(q));
        }
        return fins;
}
}
#endif
