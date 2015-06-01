#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
using namespace std;
typedef wstring str;
typedef wchar_t chr;
typedef const wchar_t cchr;
using namespace jsonld;
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

wchar_t t[4096];
list<quad> parse_nqline(const wchar_t* s) {
	list<quad> r;
	uint pos = 0;
	str graph;
	pnode subject, pred;
	std::list<pnode> objects;
	while(*s) {
		while (iswspace(*s)) ++s;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0;
			subject = mkiri(wstrim(t));
			++s;
		}
		else if (*s == L'_') {
			while (!iswspace(*s)) t[pos++] = *s++;
			t[pos] = 0; pos = 0;
			subject = mkbnode(wstrim(t));
		}
		else throw runtime_error("expected iri or bnode subject");
		while (iswspace(*s)) ++s;
	
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0;
			pred = mkiri(wstrim(t));
			++s;
		}
		else if (*s == L'=' && *++s == L'>') {
			pred = mkiri(implication);
			++s;
		}
		else throw runtime_error("expected iri predicate");
		while (iswspace(*++s));

		do {
			while (iswspace(*s)) ++s;
			if (*s == L'<') {
				while (*++s != L'>') t[pos++] = *s;
				t[pos] = 0; pos = 0;
				objects.push_back(mkiri(wstrim(t)));
				++s;
			}
			else if (*s == L'_') {
				while (!iswspace(*s)) t[pos++] = *s++;
				t[pos] = 0; pos = 0;
				objects.push_back(mkbnode(wstrim(t)));
			} else if (*s++ == L'\"') {
				do {
					t[pos++] = *s++;
				} while (!(*(s-1) != L'\\' && *s == L'\"'));
				str dt, lang;
				++s;
				while (!iswspace(*s) && *s != L'.') {
					if (*s == L'^' && *++s == L'^') {
						if (*++s == L'<')  {
							while (*s != L'>') 
								dt += *s++;
							++s;
							break;
						}
					} else if (*s == L'@') { 
						while (!iswspace(*s)) 
							lang += *s++;
						break;
					}
					else throw runtime_error("expected langtag or iri");
				}
				t[pos] = 0; pos = 0;
				objects.push_back(mkliteral(wstrim(t), pstrtrim(dt), pstrtrim(lang)));
			}
			else throw runtime_error("expected iri or bnode or literal object");
			while (iswspace(*s)) ++s;
		} while (*s++ == L',');
		--s;
		if (*s != L'.' && *s) {
			if (*s == L'<')
				while (*++s != L'>') t[pos++] = *s;
			else if (*s == L'_')
				while (!iswspace(*s)) t[pos++] = *s++;
			else throw runtime_error("expected iri or bnode graph");
			t[pos] = 0; pos = 0;
			trim(graph = t);
			++s;
			for (pnode object : objects) r.emplace_back(subject, pred, object, graph);
		} else
			for (pnode object : objects) r.emplace_back(subject, pred, object, L"@default");
		objects.clear();
		while (*s == '.') ++s;
	}
	return r;
}
