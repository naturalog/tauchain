#include "ast.h"

void readdoc(bool query);
//extern vector<ast::term*> terms;
//extern vector<ast::rule*> rules;
extern ast st;
ast::term* mktriple(ast::term* s, ast::term* p, ast::term* o);
ostream &operator<<(ostream &, const ast::rule &);
ostream &operator<<(ostream &, const ast::rule::premise &);
typedef ast::term* pterm;
typedef const ast::term* pcterm;
