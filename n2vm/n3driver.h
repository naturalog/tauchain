#ifndef __n3driver_h__
#define __n3driver_h__

#include "containers.h"
#include <map>
using std::function;
using std::wistream;
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

struct din_t {
        din_t(wistream&);
	wistream &is;
	void readdoc(bool query, struct n2vm&, vector<rule> &rules); // TODO: support prefixes
	void print(); //print kb
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
        const term* readlist(n2vm&);
        const term* readany(n2vm&);
        const term* readtriple(n2vm&);
};

#endif
