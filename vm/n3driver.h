extern "C" {
#include "strings.h"
}
#include "containers.h"
#include <map>
using std::function;
using std::ostream;
using std::string;
using std::stringstream;
using std::endl;
using std::cin;
using std::tuple;
using std::make_tuple;
using std::runtime_error;
extern ostream &dout;

typedef const char cch;
typedef cch* pcch;

//namespace n3driver {

// TODO:
// 1. debruijn
// 2. no var twice - rule (not term!) should be equipped with sub
// 3. take two terms and return two terms. take two envs and return two envs.
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
	const uint sz; // used also to freshen vars
	ostream& operator>>(ostream &os) const;
};
term* mktriple(term* s, term* p, term* o);
typedef term* pterm;
typedef const term* pcterm;

//bool unify(term *s, term *d, term **rs, term **rd) {
//}

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
