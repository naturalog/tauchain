#include "defs.h"
#include "ir.h"
#include "n3.h"
#include "eq.h"

#define eithervar(x, y) ((x).type == '?' || (y).type == '?')

char compile_term(int ***e, int x, int y) {
	term s = terms[x], d = terms[y];
	if (!eithervar(s, d)) {
		if (s.type != d.type) return 0;
		if (s.type != '.' && s.type != 'T') return x == y ? 1 : 0;
		if (*s.args++ != *d.args++) return 0;
		while (*s.args)
			if (!compile_term(e, *s.args++, *d.args++))
				return 0;
		return 1;
	}
	return require(e, x, y);
}

// compile single premise vs single conclusion
char compile_pc(int r, int p, int r1, int c) {
	assert(c && r && r1);
	assert(r < nrs && r1 < nrs);
	assert(p < rules[r].np);
	if (!rules[r1].c) return 0; // r1 is query
	assert(c <= *rules[r1].c);
	premise* prem = &rules[r].p[p];
	assert(prem->p);
	int *conc = &rules[r1].c[c];
	assert(*conc);
	term s = terms[prem->p], d = terms[*conc];
	if (!prem->e)
		(prem->e = CALLOC(int***, nrs))[r1] = CALLOC(int**, *rules[r1].c);
	int ***e = &prem->e[r1][c - 1];
	wprintf(L"r:%d p:%d r1:%d c:%d r[r1].c[0]:%d\n", r, p, r1, c, *rules[r1].c), fflush(stdout);
	assert(e);
	if (!*e)
		*e = create_relation();
	if (!compile_term(e, prem->p, *conc))
		return free(**e), free(*e), *e = 0, 0;
	wprintf(L"equality constraints for "), print(&s),
	wprintf(L" vs "), print(&d),
	wprintf(L" is:\n"), printc(*e);
	return 1;
}
void compile() {
	for (int r = 1; r < nrs; ++r)
		for (int p = 0; p < rules[r].np; ++p)
			for (int r1 = 1; r1 < nrs; ++r1)
				if (rules[r1].c)
					for (int c = 1; c <= *rules[r1].c; ++c)
						compile_pc(r, p, r1, c);
}

int _main(/*int argc, char** argv*/) {
	const size_t buflen = 256;
	setlocale(LC_ALL, "");
	mkterm(0, 0), mkrule(0, 0, 0); // termerve zero indices to signal failure
//	test();
	int pos = 0;
	input = MALLOC(wchar_t, buflen);
	while (!feof(stdin)) { // read whole doc into mem
		if ((input[pos++] = getwchar()) == (wchar_t)WEOF) break;
		if (!(pos % buflen))
			input = REALLOC(input, buflen * (1 + pos / buflen), wchar_t);
	}
	input[--pos] = 0;
	wchar_t *_input = input;
	parse();
	free(_input); // input doc can be free'd right after parulese
	for (int n = 1; n < nrs; ++n) printr(&rules[n]), putws(L"");
	compile();
//	for (int n = 1; n < nrules; ++n) {
//		printr(&rules[n]);
//		for (int b = 0; b < rules[n].np; ++b)
//			for (int k = 0; k < nrules; ++k)
//				if (rules[k].c) for (int m = 0; m < *rules[k].c; ++m)
//					if(rules[n].p[b].e && rules[n].p[b].e[k])
//						printc(rules[n].p[b].e[k][m]);
//		putws(L"");
//	}
	return 0;
}
