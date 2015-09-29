#ifndef __CLI_H__
#define __CLI_H__

#include "rdf.h"

extern bool fnamebase, quad_in, nocolor;


namespace old{

typedef std::vector<string> strings;


class cmd_t {
protected:
#ifdef JSON
	pobj load_json ( string fname = L"", bool print = false );
	pobj load_json ( const strings& args );
	pobj nodemap ( const strings& args );
	pobj nodemap ( pobj o );
	qdb toquads ( const strings& args );
	qdb toquads ( pobj o );
	qdb convert ( pobj o );
	qdb convert ( const string& s );
#endif
#ifndef NOPARSER
	std::shared_ptr<qdb> load_quads ( string fname, bool print = true );
#endif
public:
	virtual std::string desc() const = 0;
	virtual std::string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;
};

typedef std::pair<std::map<string, cmd_t*>, std::map<std::pair<string, string>, bool*>>  cmds_t;

void print_usage ( const cmds_t& cmds );
void process_flags ( const cmds_t& cmds, strings& args );
#ifdef JSON
extern jsonld_options opts;
class convert_cmd : public cmd_t {
public:
	virtual std::string desc() const;
	virtual std::string help() const;
	virtual int operator() ( const strings& args );
};
#endif

}

#endif
