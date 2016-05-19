extern "C" {
#include "strings.h"
}
#include "containers.h"
#include <map>
#include <forward_list>
#include <list>
#include <set>
using std::function;
using std::ostream;
using std::string;
using std::stringstream;
using std::endl;
using std::cin;
using std::tuple;
using std::make_tuple;
using std::runtime_error;
using std::forward_list;
using std::set;
extern ostream &dout;

typedef const char cch;
typedef cch* pcch;

//namespace n3driver {
struct crd {
	std::list<int> c;
	pcch str;
//	bool var;
	crd(pcch str) : str(str) {} //, var(str && *str == '?') {}
};
typedef set<crd, struct ccmp> word;

struct ccmp { int operator()(const crd& x, const crd& y); };

template<typename T, typename V>
T push_front(const T& t, const V& i) {
	T r;
	for (auto x : t) {
		x.c.push_front(i);
		r.insert(x);
	}
	return r;
}
ostream& operator<<(ostream& os, const crd& c);
ostream& operator<<(ostream& os, const word& w);

#define mkterm(x) new term(x)

class term {   // iri, var, or list
        pcch p;
	term **args;
public:
	enum ttype { var, lst, lit, etc };
        term(pcch v);
        term(term** _args, uint sz);
	term(const vector<term*>& a) : term(a.begin(), a.size()) {}
        ~term();
//	const ttype t;
	const uint sz;
	ostream& operator>>(ostream &os) const;
	void crds(word&, crd c = crd(0), int k = -1) const;
};

struct premise {
        const term *t;
        premise(const term *t);
	ostream &operator>>(ostream &os) const;
};
typedef vector<premise*> body_t;

struct rule {
        const term *head;
	const body_t body;
        rule(const term *h, const body_t &b = body_t());
        rule(const term *h, const term *b);
        rule(const rule &r);
	ostream &operator>>(ostream &os) const;
	word crds(int rl);
};

