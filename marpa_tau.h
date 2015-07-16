#include "cli.h"

const pnode marpa_parser = mkiri(pstr(L"http://idni.org/marpa#parser"));
const pnode marpa_parse = mkiri(pstr(L"http://idni.org/marpa#parse"));

class load_n3_cmd : public cmd_t{
public:
	virtual std::string desc() const;
	virtual std::string help() const;
	virtual int operator() ( const strings& args );
};

struct Marpa;