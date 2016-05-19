#include "ast.h"

void readdoc(bool query); // TODO: support prefixes
extern vector<term*> terms;
extern vector<rule*> rules;
extern map<const term*, int> dict;
//extern din_t din;
term* mktriple(term* s, term* p, term* o);
ostream &operator<<(ostream &, const rule &);
ostream &operator<<(ostream &, const premise &);
typedef term* pterm;
typedef const term* pcterm;
//}
