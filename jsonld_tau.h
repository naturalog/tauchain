#ifndef __CLI_H__
#define __CLI_H__

#include "rdf.h"
#include "misc.h"

#ifdef JSON

//extern bool fnamebase, nocolor;
//typedef std::vector<string> strings;

ParsingResult load_jsonld ( qdb &kb, std::istream &is );

#endif

#endif
