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
	str wt;
	while(*s) {
		while (iswspace(*s)) ++s;
		pnode subject;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0;
			trim(wt = t);
			subject = mkiri(wt);
			++s;
		}
		else if (*s == L'_') {
			while (!iswspace(*s)) t[pos++] = *s++;
			t[pos] = 0; pos = 0;
			trim(wt = t);
			subject = mkbnode(wt);
		}
		else throw runtime_error("expected iri or bnode subject");
		while (iswspace(*s)) ++s;
	
		pnode pred;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0;
			trim(wt = t);
			pred = mkiri(wt);
			++s;
		}
		else if (*s == L'=' && *++s == L'>') {
			pred = mkiri(implication);
			++s;
		}
		else throw runtime_error("expected iri predicate");
		while (iswspace(*++s));
	
		pnode object;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0;
			trim(wt = t);
			object = mkiri(wt);
			++s;
		}
		else if (*s == L'_') {
			while (!iswspace(*s)) t[pos++] = *s++;
			t[pos] = 0; pos = 0;
			trim(wt = t);
			object = mkbnode(wt);
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
			trim(wt = t);
			trim(dt);
			trim(lang);
			object = mkliteral(wt, pstr(dt), pstr(lang));
		}
		else throw runtime_error("expected iri or bnode or literal object");
		str graph;
		while (iswspace(*s)) ++s;
		if (*s != L'.' && *s) {
			if (*s == L'<') {
				while (*++s != L'>') t[pos++] = *s;
				t[pos] = 0; pos = 0;
				trim(graph = t);
			}
			else if (*s == L'_') {
				while (!iswspace(*s)) t[pos++] = *s++;
				t[pos] = 0; pos = 0;
				trim(graph = t);
			}
			else throw runtime_error("expected iri or bnode graph");
			++s;
			r.emplace_back(subject, pred, object, graph);
		} else
			r.emplace_back(subject, pred, object, L"@default");
		while (*s == '.') ++s;
	}
	return r;
}
