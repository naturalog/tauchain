#include "defs.h"
#include "ir.h"
#include "n3.h"
#include "eq.h"

#define eithervar(x, y) ((x).type == '?' || (y).type == '?')

char compile_res(int **e, int x, int y) {
	struct res s = rs[x], d = rs[y];
	if (!eithervar(s, d)) {
		if (s.type != d.type) return 0;
		if (s.type != '.') return x == y ? 1 : 0;
		if (*s.args++ != *d.args++) return 0;
		while (*s.args)
			if (!compile_res(e, *s.args++, *d.args++))
				return 0;
		return 1;
	}
	return require(&e, x, y);
}

// compile single premise vs single conclusion
char compile_pc(int r, int p, int r1, int c) {
	struct triple s = ts[rls[r].p[p].p];
	struct triple d = ts[rls[r1].c[c]];
	if (!rls[r].p[p].e) rls[r].p[p].e = MALLOC(int***, nrls);
	rls[r].p[p].e[r1] = MALLOC(int**, *rls[r1].c);
	int ***e = &rls[r].p[p].e[r1][c];
	*e = create_relation();
	if (! 	compile_res(*e, s.p, d.p) &&
		compile_res(*e, s.s, d.s) &&
		compile_res(*e, s.o, d.o))
		return 0;
	wprintf(L"equality relation for "), printt(&s),
	wprintf(L" vs "), printt(&d),
	wprintf(L" is:\n"), printc(*e);
	return 1;
}
void compile_premise(int r, int p) {
	for (int r1 = 1; r1 < nrls; ++r1)
		for (int c = 0; c < *rls[r].c; ++c)
			compile_pc(r, p, r1, c);
}
void compile_rule(int r) {
	for (int p = 0; p < rls[r].np; ++p) compile_premise(r, p);
}
void compile() {
	for (int r = 1; r < nrls; ++r) compile_rule(r);
}

int _main(/*int argc, char** argv*/) {
	const size_t buflen = 256;
	setlocale(LC_ALL, "");
	mkres(0, 0), mktriple(0, 0, 0), mkrule(0, 0, 0); // reserve zero indices to signal failure
//	test();
//	return 0;
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
	free(_input); // input doc can be free'd right after parse
	compile();
	for (int n = 1; n < nrls; ++n) {
		printr(&rls[n]);
		for (int b = 0; b < rls[n].np; ++b)
			for (int k = 0; k < nrls; ++k)
				if (rls[k].c) for (int m = 0; m < *rls[k].c; ++m)
					if(rls[n].p[b].e)
						if(rls[n].p[b].e[k])
							printc(rls[n].p[b].e[k][m]);
		putws(L"");
	}
	return 0;
}
