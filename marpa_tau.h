#include "cli.h"
#include <istream>

int parse_natural3(qdb &kb, qdb &q, std::wistream &f, string base=L"@default");

void* marpa_parser(prover*, nodeid, shared_ptr<prover::proof>);
termid marpa_parse(void*, string);
string load_file(std::ifstream &);
