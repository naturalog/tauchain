#ifndef __CLI_H__
#define __CLI_H__

#include <string>
#include <vector>
#include "rdf.h"

using namespace jsonld;

typedef vector<string> strings;

class cmd_t {
//	jsonld::jsonld_options opts;
public:
	virtual string desc() const = 0;
	virtual string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;

	pobj load_json ( string fname = "", bool print = false );
	pobj load_json ( const strings& args );
	pobj nodemap ( const strings& args );
	pobj nodemap ( pobj o, pstring base );
	qdb toquads ( const strings& args );
	qdb toquads ( pobj o, pstring base );
	qdb convert ( pobj o, pstring base );
	qdb convert ( const string& s, pstring base = 0);
};

void print_usage ( const map<string, cmd_t*>& cmds );

#endif
