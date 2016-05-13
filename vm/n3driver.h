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

namespace n3driver {

struct term {   // iri, var, or list
        pcch p;
	term **args;
	const bool var, lst;
	const uint sz; // used also to freshen vars
        term(pcch v);
        term(term** _args, uint sz);
	term(const vector<term*>& a) : term(a.begin(), a.size()) {}
        ~term();
};
term* mktriple(term* s, term* p, term* o);

struct premise {
        const term *t;
        premise(const term *t);
};
typedef vector<premise> body_t;

struct rule {
        const term *head;
        body_t body;
        rule(const term *h, const body_t &b = body_t());
        rule(const term *h, const term *b);
        rule(const rule &r);
};

ostream &operator<<(ostream &, const term &);
ostream &operator<<(ostream &, const term &t);
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
extern vector<rule> rules;
extern map<const term*, int> dict;
extern din_t din;
}
