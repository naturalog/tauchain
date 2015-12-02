#ifndef __MARPA_TAU_H__
#define __MARPA_TAU_H__

#include "prover.h"
#include "jsonld_tau.h"
#include <istream>
#include "misc.h"

ParsingResult parse_natural3(qdb &kb, qdb &q, std::istream &f, string base = "@default");

#endif
