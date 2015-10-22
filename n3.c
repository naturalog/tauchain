#include "defs.h"
#include "ir.h"
#include "n3.h"

wchar_t *input;
char in_query = 0; // 1 after first "fin."
const wchar_t* getstr(const wchar_t *s); // implemented in dict.cpp, manages unique strings

void skip() { while (*input && iswspace(*input)) ++input; }
const wchar_t* take(int p) {
	wchar_t s[p + 1];
	wcsncpy(s, input, p), s[p] = 0, input += p;
	return getstr(s);	
}
char peek(const wchar_t *_s) {
	const wchar_t *s = _s;
	int n = 0;
	while (*s) if (!input[n] || input[n++] != *s++) return 0;
	return 1;
}
void expect(const wchar_t *s) {
	if (skip(), !peek(s)) wprintf(L"Error: \"%s\" expected.", s), exit(1);
	input += wcslen(s);
}

int* getlist(int (*f)(), wchar_t till) {
	int t, *args = CALLOC(int, 2), sz;
	while (skip(), *input != till) {
		if (!(t = f())) putws(L"Unexpected item: "), putws(input), exit(1);
		sz = args ? *args : 0;
		args = REALLOC(args, ++sz + 2, int);
		args[*args = sz] = t;
		args[sz + 1] = 0;
	}
	return ++input, args;
}
int getres() {
	skip();
	int p = wcscspn(input, L" \r\n\t(){}.");
	switch (input[p]) {
	case 0: return 0;
	case L'{': putws(L"Unexpected {:"), putws(input); exit(1);
	case L'(': return ++input, mkres(getlist(getres, L')'), '.');
	default: return mkres(take(p), 0);
	}
}
int gettriple() {
	int s, p, o;
	if (!((s = getres()) && (p = getres()) && (o = getres()))) return 0;
	if (skip(), *input == L'.') ++input, skip();
	int t = mktriple(s, p, o);
	return t;
}
premise* topremises(int* p) {
	premise *r = MALLOC(premise, *p);
	for (int n = 1; n <= *p; ++n) r[n - 1].p = p[n], r[n - 1].e = 0;
	assert(p[1 + *p] == 0);
	free(p);
	return r;
}
int getrule() {
	int r;
	premise *p;
	if (skip(), peek(L"fin.")) {
		if (!in_query) return in_query = 1, input += 4, getrule();
		return 0;
	}
	if (skip(), !*input) return 0;
	if (*input != L'{') {
		int r;
		if (!(r = gettriple())) return 0;
		if (in_query) {
			(p = MALLOC(premise, 1))->p = r, p->e = 0;		
			return mkrule(p, 1, 0);
		}
		int *rr = CALLOC(int, 3);
		rr[rr[rr[2] = 0] = 1] = r;
		return mkrule(0, 0, rr);
	}
	++input;
	int *pp = getlist(gettriple, L'}');
	expect(L"=>"), skip(), expect(L"{");
	int *cc = getlist(gettriple, L'}');
	r = mkrule(topremises(pp), *pp, cc);
	if (*input == L'.') ++input;
	return r;
}
void parse() {
	int r;
	while ((r = getrule()));// printr(&rls[r]), putwchar(L'\n');
}
