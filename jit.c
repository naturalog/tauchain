#include "defs.h"
#include "ir.h"
#include "n3.h"
#include "eq.h"

char canmatch(int x, int y) {
	struct res s = rs[x], d = rs[y];
	if (!eithervar(s, d)) {
		if (s.type != d.type) return 0;
		if (s.type != '.') return x == y ? 1 : 0;
		if (*s.args++ != *d.args++) return 0;
		while (*s.args) if (!canmatch(*s.args++, *d.args++)) return 0;
		return 1;
	}
	
//	return setcond(x, y);
	return 1;
}

int _main(/*int argc, char** argv*/) {
	const size_t buflen = 256;
	setlocale(LC_ALL, "");
	mkres(0, 0), mktriple(0, 0, 0), mkrule(0, 0); // reserve zero indices to signal failure
	test();
	return 0;
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
	return 0;
}
