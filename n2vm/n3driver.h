#include "containers.h"
#include <map>
using std::function;
using std::wostream;
using std::wstring;
using std::stringstream;
using std::endl;
using std::wcin;
using std::tuple;
using std::make_tuple;
using std::runtime_error;
extern wostream &dout;

namespace n3driver {

struct term {   // iri, var, or list
        wchar_t type; // the only member used in runtime
        union {
                int *args; // list items, last item must be null.
                const wchar_t *val;
        };
        term(const wchar_t *v);
        term(vector<int> _args);
        ~term();
};

struct triple {
        int r[3]; // spo
        triple(int s, int p, int o);
        triple(const triple &t);
};

struct premise {
        const triple *t;
        premise(const triple *t);
};
typedef vector<premise> body_t;

struct rule {
        const triple *head;
        body_t body;
        rule(const triple *h, const body_t &b = body_t());
        rule(const triple *h, const triple *b);
        rule(const rule &r);
};

wostream &operator<<(wostream &, const term &);
wostream &operator<<(wostream &, const triple &t);
wostream &operator<<(wostream &, const rule &);
wostream &operator<<(wostream &, const premise &);
int mkterm(const wstring &v);
int mkterm(const vector<int> &v);
const triple *mktriple(int s, int p, int o);

struct din_t {
        din_t();
        void readdoc(bool query); // TODO: support prefixes
private:
        wchar_t ch;
        bool f = false, done, in_query = false;
        wchar_t peek();
        wchar_t get();
        wchar_t get(wchar_t c);
        bool good();
        void skip();
        wstring getline();
        wstring &trim(wstring &s);
        static wstring edelims;
        wstring till();
        int readlist();
        int readany();
        const triple *readtriple();
};

extern vector<term*> terms;
extern vector<rule> rules;
extern map<const term*, int> dict;
extern din_t din;
}
