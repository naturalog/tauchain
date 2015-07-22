#include "cli.h"

class load_n3_cmd : public cmd_t{
public:
	virtual std::string desc() const;
	virtual std::string help() const;
	virtual int operator() ( const strings& args );
};

void* marpa_parser(prover*, resid, prover::proof*);
prover::termid marpa_parse(void*, string);

