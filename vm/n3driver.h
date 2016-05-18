extern "C" {
#include "strings.h"
}
#include "containers.h"
#include <map>
#include <forward_list>
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
typedef forward_list<int> crd;
typedef set<crd, struct ccmp> word;

struct ccmp {
	int operator()(const crd& x, const crd& y) {
		auto ix = x.begin(), ex = x.end();
		auto iy = y.begin(), ey = y.end();
		while (ix != ex && iy != ey)
			if (*ix != *iy) return *ix < *iy ? 1 : -1;
			else ++ix, ++iy;
		return 0;
	}
};

template<typename T, typename V>
void push_front(T& t, const V& i) { for (auto x : t) x.push_front(i); }
inline ostream& operator<<(ostream& os, const crd& c) {
	for (int i : c) os << i << ' '; return os;
}
inline ostream& operator<<(ostream& os, const word& w) {
	for (auto c : w)os << "( " << c << ')'; return os << endl;
}

class term {   // iri, var, or list
        pcch p;
	term **args;
public:
	enum ttype { var, lst, lit, etc };
        term(pcch v);
        term(term** _args, uint sz);
	term(const vector<term*>& a) : term(a.begin(), a.size()) {}
        ~term();
	const ttype t;
	const uint sz;
	ostream& operator>>(ostream &os) const;

	void crds(word &l, word &v, crd c = crd(), int k = -1) const {
		if (k != -1) c.push_front(k);
		if (t == lit) { l.emplace(c); return; }
		if (t == var) { l.emplace(c); return; }
		for (uint n = 0; n < sz; ++n) args[n]->crds(l, v, c, n);
	}
};
term* mktriple(term* s, term* p, term* o);
typedef term* pterm;
typedef const term* pcterm;

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
	void crds(word &l, word &v) {
		if (head) head->crds(l, v), push_front(l, -1), push_front(v, -1);
		for (uint n = 0; n < body.size(); ++n) {
			word _l, _v;
			body[n]->t->crds(_l, _v), push_front(l, n), push_front(v, n);
		}

	}
};

//ostream &operator<<(ostream &, const term &);
//ostream &operator<<(ostream &, const term &t);
ostream &operator<<(ostream &, const rule &);
ostream &operator<<(ostream &, const premise &);

struct din_t {
        din_t();
        void readdoc(bool query); // TODO: support prefixes
private:
        char ch;
        bool f = false, done, in_query = false;
        char peek();
        char get();
        char get(char c);
        bool good();
        void skip();
        void getline();
        string &trim(string &s);
        static string edelims;
        string till();
        term* readlist();
        term* readany();
        const term *readterm();
	bool eof;
};

extern vector<term*> terms;
extern vector<rule*> rules;
extern map<const term*, int> dict;
extern din_t din;
//}
