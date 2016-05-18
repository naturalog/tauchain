#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <forward_list>
using namespace std;

#define umap unordered_map
#define uset unordered_set
#define flist forward_list
#define two(x) pair<x, x>
#define fst first
#define snd second
#define iter iterator

class rule {
	umap<pcch, lit> 
public:
	bool operator()();
};

typedef map<crd, int> rule;

void add(rule r, body b, crd c, int i) {
	r[prepend(c, b)] = i;
}
void add(rule rb, body b, rule rh, head h) {
	auto &rcb = r[prepend(c, b)];
	auto &rch = r[prepend(c, b)];
}

void match(	rule b, rule h
		, rule::iter ib, rule::iter ih
		, rule::iter eb, rule::iter eh) {
	while (ib != eb && ih != eh) {
		if (ib->snd > 0 && ih->snd > 0) {
			if (
		}
	}
		
}

typedef const char* crd;
typedef const char* lit;
typedef uint rule;
typedef uint head;
typedef uint body;
typedef uint rhb;
typedef uint blank;
typedef uint word;
typedef pair<rule, uint> rhead;
typedef pair<rule, uint> rbody;
typedef pair<rule, uint> rhb;
typedef pair<crd, lit> crdl;
/*
	Possible Refutations:

	1. When body try to match a head:
		The two words have two symbols in which one is
		a strict prefix of the other.
	2. When rule mutate due to match:
		If the excess symbols appear in the vars dsds,
		and a member of this ds is mapped to a different lit.
	3. When deeper levels fail.

*/

// refuted
uset<two(crd)> pfs; // prefix refutations
uset<two(word*)> rws; // word merge refutation
map<rhb, umap<crd, lit>> neq; // dsds of lit coords in a rule per term
// matches
multimap<word*, rhead> m;
// structure
umap<word, set<crd>> ws;
umap<rule, umap<head, word*> hs;
umap<rule, umap<body, word*> bs;
umap<uint, map<crd, blank>> eqs; // vars dsds in a rule

// proof
umap<word*, flist<rule>> ep;

rule nrl = 0;

// exceptions
struct prefix_refutation : public std::exception {};
struct literal_refutation : public std::exception {};
struct backtrack : public std::exception {};
struct dead_code : public std::exception {};
struct euler_path : public std::exception {};
struct occur_check : public std::exception {};

// iface
bool prove(rbody g);
void add(word &w, crd c) throw (prefix_refutation);
void addneq(rbody b, crd c, lit l) throw (literal_refutation);
void addeq(rule r, crd c, lit l);
void add(rbody b, crd c, lit l);
void add(rule r, crd c, lit l) throw (backtrack);
void update_matches(word*, word*) throw (dead_code);
void epcheck(rule) throw (euler_path);
void occheck(rule, crd) throw (occur_check);

// impl
bool prove(rbody g) {
	auto it = m.equal_range(bs[g]);
	bool res = false;
	while (it.fst != it.snd) {
		word *w = hs[it.fst.snd];
		it = rws.find(bs[g], w);
		if (it != rws.end()) continue;
		try {
		} catch (...) {
		}

		res = true;
	}
	return res;
}

void add(word &w, crd c) {
	for (crd s : w)
		if (pfs.find(s, c) != pfs.end())
			throw 0;
}

void addneq(rbody b, crd c, lit l) {
	auto &nb = neq[b];
	auto it = nb.find(c);
	if (it == nb.end()) {
		nb[c] = l;
		return;
	}
	if (it->snd != l) throw 0;
}

void addeq(rule r, crd c, lit l) {
	auto &e = eqs[r];
	auto it = e.find(c);
	if (it == e.end()) return;
	for (auto x : it->snd)
		add(b.fst, x.fst, l);
	e.erase(it);
}

void add(rule r, crd c, lit l) { }

}

void add(rbody b, crd c, lit l) {
	word *w = bs[b];
	add(*w, c);
	addneq(b, c, l);
	addeq(b.fst, c, l);
	update_matches();
}
