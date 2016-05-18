#include "ast.h"

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
term* mktriple(term* s, term* p, term* o);
ostream &operator<<(ostream &, const rule &);
ostream &operator<<(ostream &, const premise &);
typedef term* pterm;
typedef const term* pcterm;
//}
