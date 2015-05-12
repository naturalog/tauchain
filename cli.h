#ifndef __CLI_H__
#define __CLI_H__

#include <string>
#include <vector>
#include "rdf.h"

using namespace jsonld;

typedef vector<string> strings;

extern bool fnamebase, quad_in;
extern jsonld::jsonld_options opts;
extern string chan;

class cmd_t {
protected:
	pobj load_json ( string fname = "", bool print = false );
	pobj load_json ( const strings& args );
	pobj nodemap ( const strings& args );
	pobj nodemap ( pobj o );
	qdb toquads ( const strings& args );
	qdb toquads ( pobj o );
	qdb convert ( pobj o );
	qdb convert ( const string& s, bool bdebugprint = false );
	qdb load_quads ( string fname, bool print = true );
public:
	virtual string desc() const = 0;
	virtual string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;
};

typedef pair<map<string, cmd_t*>, map<pair<string, string>, bool*>>  cmds_t;

void print_usage ( const cmds_t& cmds );
void process_flags ( const cmds_t& cmds, strings& args );

#ifdef marpa
class load_n3_cmd : public cmd_t {
public:
	virtual string desc() const;
	virtual string help() const;
	virtual int operator() ( const strings& args );
};
#endif

class expand_cmd : public cmd_t {
public:
	virtual string desc() const;
	virtual string help() const;
	virtual int operator() ( const strings& args );
};

class convert_cmd : public cmd_t {
public:
	virtual string desc() const;
	virtual string help() const;
	virtual int operator() ( const strings& args );
};

class toquads_cmd : public cmd_t {
public:
	virtual string desc() const;
	virtual string help() const;
	virtual int operator() ( const strings& args );
};

class nodemap_cmd : public cmd_t {
public:
	virtual string desc() const;
	virtual string help() const;
	virtual int operator() ( const strings& args );
};

#endif
