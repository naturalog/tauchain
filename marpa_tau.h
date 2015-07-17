#include "cli.h"

const pnode marpa_parser_iri = mkiri(pstr(L"http://idni.org/marpa#parser"));
const pnode marpa_parse_iri = mkiri(pstr(L"http://idni.org/marpa#parse"));

class load_n3_cmd : public cmd_t{
public:
	virtual std::string desc() const;
	virtual std::string help() const;
	virtual int operator() ( const strings& args );
};

void* marpa_parser(prover*, resid, prover::proof);
pnode marpa_parse(void*, string);

