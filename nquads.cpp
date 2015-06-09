#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

wchar_t t[4096];
std::list<quad> parse_nqline(const wchar_t* s) {
	std::list<quad> r;
	uint pos = 0;
	string graph;
	pnode subject, pn;
	typedef std::list<pnode> plist;
	std::list<std::pair<pnode, plist>> preds;

	auto readiri = [&]() {
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0;
			++s;
			return mkiri(wstrim(t));
		}
		return (pnode)0;
	};
	auto readbnode = [&]() {
		if (*s == L'_') {
			while (!iswspace(*s)) t[pos++] = *s++;
			t[pos] = 0; pos = 0;
			return mkbnode(wstrim(t));
		}
		return (pnode)0;
	};

	while(*s) {
		while (iswspace(*s)) ++s;
		if (!(subject = readiri()) && !(subject = readbnode()))
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
				if ((pn = readiri()) || (pn = readbnode()))
					preds.back().second.push_back(pn);
				else if (*s++ == L'\"') {
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
					preds.back().second.push_back(mkliteral(wstrim(t), pstrtrim(dt), pstrtrim(lang)));
				}
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
			graph = L"@default";
		for (auto x : preds) for (pnode object : x.second) r.emplace_back(subject, x.first, object, graph);
		preds.clear();
		while (*s == '.') ++s;
	}
	return r;
}
