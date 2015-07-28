#ifndef __CLI_H__
#define __CLI_H__

#include "rdf.h"

//COMMAND LINE
/*
Now all those extern command line variables declared here like they should be, and are available like we want/expect
*/
extern jsonld_options opts;

extern bool autobt, _pause, __printkb, fnamebase, quad_in, nocolor, deref, shorten;


//This is mine, just a test
extern std::string test_loadn3;

//FILE-IO & FORMATTING
class cmd_t {
protected:
	pobj load_json ( string fname = L"", bool print = false );
	pobj load_json ( const strings& args );
	pobj nodemap ( const strings& args );
	pobj nodemap ( pobj o );
	qdb toquads ( const strings& args );
	qdb toquads ( pobj o );
	qdb convert ( pobj o );
	qdb convert ( const string& s );
	std::shared_ptr<qdb> load_quads ( string fname, bool print = true );
public:
	virtual std::string desc() const = 0;
	virtual std::string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;
};



typedef std::pair<std::map<string, cmd_t*>, std::map<std::pair<string, string>, bool*>>  cmds_t;

//COMMAND LINE
void print_usage ( const cmds_t& cmds );
void process_flags ( const cmds_t& cmds, strings& args );

//FILE-IO & FORMATTING
class convert_cmd : public cmd_t {
public:
	virtual std::string desc() const;
	virtual std::string help() const;
	virtual int operator() ( const strings& args );
};
#endif
