#include <map>
#include <vector>
#include <utility>
#include <functional>
using namespace std;

enum etype { IRI, VAR, BNODE, LITERAL, LIST };
struct resource {
	etype type;
	union {
		resource* sub;	// substitution, in case of var.
				// not used in compile time
		resource** args;// list items. last item must be null
	};
};
vector<resource> res;
typedef map<resource*, resource*> subs;

struct triple {
	resource *r[3]; // spo
};

struct rule {
	triple head;
	vector<triple> body;
};
vector<rule> rules;

bool occurs_check(resource *x, resource *y) {
	if (x->type != VAR) return (y->type != VAR) ? false : occurs_check(y, x);
	if (y->type != LIST) return false;
	resource **r = y->args;
	while (*r)
		if (occurs_check(x, *r++))
			return true;
	return false;
}

bool prepare(resource *s, resource *d, subs& c) {
	if (s->type != VAR && d->type != VAR) {
		if (s != d) return c.clear(), false;
		if (s->type != LIST) return true;
		resource **rs, **rd;
		for (rs = s->args, rd = d->args; *rs && *rd;)
			if (!*rs != !*rd || !prepare(*rs++, *rd++, c))
				return false;
		return true;
	}
	if (occurs_check(s, d)) return false;
	return c[s] = d, true;
}

bool prepare(triple *s, triple *d, subs& c) {
	for (int n = 0; n < 3; ++n)
		if (!prepare(s->r[n], d->r[n], c))
			return false;
	return true;
}

typedef map<int/*head*/, subs> conds;
// compiler's intermediate representation language. the information
// in the following variable contains everything the emitter has to know
map<pair<int/*head*/,int/*body*/>, conds> intermediate;

map<pair<int/*head*/,int/*body*/>, std::function<void()>> program;

void prepare() {
	subs c;
	for (int n = 0; n < rules.size(); ++n)
		for (int k = 0; k < rules[n].body.size(); ++k)
			for (int m = 0; m < rules.size(); ++m)
				if (prepare(&rules[n].body[k], &rules[m].head, c))
					intermediate[make_pair(n, k)][m] = c, c.clear();
};

const size_t max_frames = 1e+6;

struct frame {
	int h,b;
	frame* prev;
	subs trail;
} *first, *last = new frame[max_frames];

bool unify(resource *s, resource *d, subs& trail) { // in runtime
}

bool compile(int h, int b) {
	auto i = make_pair(h, b);
	conds& c = intermediate[i];
	program[i] = [c]() {
		for (conds::const_iterator x = c.begin(); x != c.end(); ++x) {
			for (pair<resource*, resource*> y : x->second)
				if (!unify(y.first, y.second, last->trail))
					continue;
			last->h = x->first, last->b = 0, last->prev = first, ++last;
		}
	};
};

void compile() {
	prepare();
	for (int n = 0; n < rules.size(); ++n)
		for (int k = 0; k < rules[n].body.size(); ++k)
			compile(n, k);
}

void run(int q /* query's index in rules */) {
	first = last;
	first->h = q, first->b = 0, first->prev = 0;
	do {
		program[make_pair(first->h, first->b)]();
	} while (++first <= last);
}

int main() {
//	read();
	int q;// = parse();
	compile();
	run(q);
}
