#include "vm.h"

#define eithervar(x, y) ((x).type == '?' || (y).type == '?')

char compile_term(uf **e, int x, int y) {
	term s = terms[x], d = terms[y];
	if (!eithervar(s, d)) {
		if (s.type != d.type)
			return 0;
		if (s.type != '.') {
			if (s.type != 'T') return s.value == d.value;
			return 	compile_term(e, s.args[0], d.args[0]) &&
				compile_term(e, s.args[1], d.args[1]) &&
				compile_term(e, s.args[2], d.args[2]);
		}
		if (*s.args++ != *d.args++)
			return 0;
		while (*s.args)
			if (!compile_term(e, *s.args++, *d.args++))
				return 0;
		return 1;
	}
	*e = unio(*e, x, y);
	return 1;
}

// compile single premise vs single conclusion
char compile_pc(int r, int p, int r1, int c) {
	if (!rules[r1].c) return 0; // r1 is query
	premise* prem = &rules[r].p[p];
	int *conc = &rules[r1].c[c];
	term s = terms[prem->p], d = terms[*conc];

	assert(	c && r && r1 && r < nrs && r1 < nrs && p < rules[r].np &&
		c <= *rules[r1].c && prem->p && *conc && s.type == 'T' && d.type == 'T');

	if (!prem->e) prem->e = CALLOC(int***, nrs);
	if (!prem->e[r1]) prem->e[r1] = CALLOC(int**, *rules[r1].c);
	uf *e = prem->e[r1][c - 1];
	//wprintf(L"r:%d p:%d r1:%d c:%d r[r1].c[0]:%d\n", r, p, r1, c, *rules[r1].c), fflush(stdout);
//	if (!*e) *e = create_relation();
//	if (!compile_term(e, prem->p, *conc))
//		return free(**e), free(*e), *e = 0, 0;

//	wprintf(L"equality constraints for "), print(&s),
//	wprintf(L" vs "), print(&d),
//	wprintf(L" are:\n"), printc(*e);
	return 1;
}

void kb_init() {

}

void compile() {
	kb_init();
	for (int r = 1; r < nrs; ++r)
		for (int p = 0; p < rules[r].np; ++p)
			for (int r1 = 1; r1 < nrs; ++r1)
				if (rules[r1].c)
					for (int c = 1; c <= *rules[r1].c; ++c)
						compile_pc(r, p, r1, c);
}

wchar_t* readall() {
	const size_t buflen = 256;
	int pos = 0;
	input = MALLOC(wchar_t, buflen);
	while (!feof(stdin)) { // read whole doc into mem
		if ((input[pos++] = getwchar()) == (wchar_t)WEOF) break;
		if (!(pos % buflen))
			input = REALLOC(input, buflen * (1 + pos / buflen), wchar_t);
	}
	return input[--pos] = 0, input;
}

void printkb() {
	for (int n = 1; n < nrs; ++n)
		printr(&rules[n]), puts("");
}

int _main(/*int argc, char** argv*/) {
	setlocale(LC_ALL, "");
	uftest();
	mkterm(0, 0), mkrule(0, 0, 0); // reserve zero indices to signal failure
	wchar_t *_input = readall();
	parse(), free(_input), // input doc can be free'd right after parse
	printkb(), compile();
	return 0;
}
