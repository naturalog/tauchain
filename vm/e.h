#include <stdbool.h>
#include <wchar.h>

typedef struct term term;
typedef struct var var;
typedef struct uri uri;
typedef struct list list;
typedef unsigned uint;
typedef uint termid;

struct var {
	void *scope;
	term *t;
	const wchar_t *name;
};

struct uri {
	const wchar_t *name;
};

struct term {
	bool leaf;
	union {
		termid r, l;
		struct {
			bool var;
			union {
				var v;
				uri u;
			};
		};
	};
};

struct undo {
	bool rank;
	uint x, y;
	undo *n;
};

void	push_change(uint, uint, bool);
termid	update(termid, termid, bool);
void	revert(undo *u = 0);
void	commit(undo *u = 0);
termid	rep(termid);
void	inc_rank(termid);
bool	merge(termid, termid, void*);
