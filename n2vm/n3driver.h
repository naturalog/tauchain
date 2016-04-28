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

wostream& operator<<(wostream&, const term&);
wostream& operator<<(wostream&, const rule&);

rule mkrule(term *h, const vector<term*> &b = vector<term*>());

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
