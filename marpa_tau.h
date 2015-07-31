#include "cli.h"

class load_n3_cmd : public cmd_t{
public:
	virtual std::string desc() const;
	virtual std::string help() const;
	virtual int operator() ( const strings& args );
	qdb load_n3(std::ifstream &f);
};

void* marpa_parser(prover*, nodeid, shared_ptr<prover::proof>);
termid marpa_parse(void*, string);
string load_file(std::ifstream &);
