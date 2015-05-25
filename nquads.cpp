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

quad parse_nqline(const wchar_t* s) {
	while (iswspace(*s)) ++s;
	str t;
	pnode subject;
	if (*s == L'<') {
		while (*++s != L'>') t += *s;
		subject = mkiri(t);
		++s;
	}
	else if (*s == L'_') {
		while (!iswspace(*s)) t += *s++;
		subject = mkbnode(t);
	}
	else throw runtime_error("expected iri or bnode subject");
	t = L"";
	while (iswspace(*s)) ++s;

	pnode pred;
	if (*s == L'<') {
		while (*++s != L'>') t += *s;
		pred = mkiri(t);
		++s;
	}
	else throw runtime_error("expected iri predicate");
	t = L"";
	while (iswspace(*++s));

	pnode object;
	if (*s == L'<') {
		while (*++s != L'>') t += *s;
		object = mkiri(t);
		++s;
	}
	else if (*s == L'_') {
		while (!iswspace(*s)) t += *s++;
		object = mkbnode(t);
	} else if (*s++ == L'\"') {
		do {
			t += *s++;
		} while (!(*(s-1) != L'\"' && *s == L'\"'));
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
				else if (*s == L'@') { 
					while (!iswspace(*s)) 
						lang += *s++;
					break;
				}
			} else throw runtime_error("expected langtag or iri");
		}
		object = mkliteral(t, pstr(dt), pstr(lang));
	}
	else throw runtime_error("expected iri or bnode or literal object");
	t = L"";
	str graph;
	while (iswspace(*s)) ++s;
	if (*s != L'.') {
		if (*s == L'<') {
			while (*++s != L'>') t += *s;
			graph = t;
		}
		else if (*s == L'_') {
			while (!iswspace(*s)) t += *s++;
			graph = t;
		}
		else throw runtime_error("expected iri or bnode graph");
		t = L"";
		return quad(subject, pred, object, graph);
	} else
		return quad(subject, pred, object, L"@default");
}
