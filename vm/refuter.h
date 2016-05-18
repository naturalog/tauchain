#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
using namespace std;

#define umap unordered_map
#define uset unordered_set
#define two(x) pair<x, x>

typedef const char* crd;
typedef const char* lit;
typedef uint rule;
typedef uint head;
typedef uint body;
typedef uint blank;
typedef pair<rule, uint> rhead;
typedef pair<rule, uint> rbody;
typedef pair<rule, uint> rhb;
typedef pair<crd, lit> crdl;
typedef set<crd> word;

// refuted
uset<two(crd)> pfs;
uset<two(word*)> rws;
map<rhb, umap<crd, lit>> neq;

// matches
multimap<word*, rhead> m;

// structure
umap<word*, set<crd, ccmp>> ws;
umap<rule, umap<head, word*> hs;
umap<rule, umap<body, word*> bs;
umap<uint, map<crd, blank>> eqs;

rule nrl = 0;

bool prove(rbody g) {
	auto it = m.equal_range(bs[g]);
	bool res = false;
	while (it.first != it.second) {
		word *w = hs[it.first.second];
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
	if (it->second != l) throw 0;
}

void addeq(rule r, crd c, lit l) {
	auto &e = eqs[r];
	auto it = e.find(c);
	if (it == e.end()) return;
	for (auto x : it->second)
		add(b.first, x.first, l);
	e.erase(it);
}

void add(rule r, crd c, lit l) { }

}

void add(rbody b, crd c, lit l) {
	word *w = bs[b];
	add(*w, c);
	addneq(b, c, l);
	addeq(b.first, c, l);
	update_matches();
}
