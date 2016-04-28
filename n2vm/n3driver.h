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

#include "n2vm.h"
using n2vm::term;

namespace n3driver {
/*
struct rule {
        const term *head;
        vector<const term*> body;
        rule(const term *h, const vector<const term*> &b = vector<const term*>());
        rule(const term *h, const term *b);
        rule(const rule &r);
};
*/
struct rule {term *h, **b; unsigned sz; operator std::wstring() const; };
rule mkrule(term *h, const vector<term*> &b = vector<term*>());

wostream &operator<<(wostream &, const term &);
wostream &operator<<(wostream &, const rule &);

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
        term* readlist();
        term* readany();
        term* readtriple();
};

extern vector<term*> terms;
extern vector<rule> rules;
extern map<const term*, int> dict;
extern din_t din;
}
