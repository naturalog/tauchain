#include "proof.h"
#include "jsonld.h"
#include "parsers.h"
#include "cli.h"
#include <boost/algorithm/string.hpp>
#ifdef IRC
#include "pstream.h"
#endif

using namespace boost::algorithm;

qdb cmd_t::load_quads ( string fname, bool print ) {
	qdb q;
	try {
		qdb r;
#ifndef IRC
		istream* pis = &cin;
#else		
		static istream* pis = new redi::ipstream(string("tail -n 0 -F /tmp/irc.freenode.net/#")+chan+"/out", redi::pstreams::pstdout);
//		pis->seekg(0, pis->end);
		if (fname != "")
			pis = new ifstream(fname);
#endif			
		istream& is = *pis;
		string line;
		stringstream ss;
		while (!is.eof()) {
			getline(is, line);
			trim(line);
			if (!line.size() || line == "\n")
				continue;
#ifdef IRC
			string magic = "botau: ";
			auto pos = line.find(magic);
			if (pos == string::npos)
				continue;
			line = line.substr(pos + magic.size());
//			cout << line;
#endif
			if (endsWith(line, "fin."))
				return readqdb(ss);
			ss << line;
		}
		if (fname != "")
			delete pis;
		return readqdb(ss);
	} catch (exception& ex) {
		derr << "Error reading quads: " << ex.what() << endl;
	}
	if ( print ) 
		dout << q << endl;
	return q;
}

pobj cmd_t::load_json ( string fname, bool print ) {
	json_spirit::mValue v;
	if ( fname == "" ) json_spirit::read_stream ( cin, v );
	else {
		ifstream is ( fname );
		json_spirit::read_stream ( is, v );
	}
	pobj r =  jsonld::convert ( v );
	if ( !r ) throw runtime_error ( "Couldn't read input." );
	if ( print ) dout << r->toString() << endl;
	return r;
}

pobj cmd_t::load_json ( const strings& args ) {
	return load_json ( args.size() > 2 ? args[2] : "" );
}

pobj cmd_t::nodemap ( const strings& args ) {
	return nodemap ( load_json ( args[2] ) );
}

pobj cmd_t::nodemap ( pobj o ) {
	psomap nodeMap = make_shared<somap>();
	( *nodeMap ) [str_default] = mk_somap_obj();
	jsonld::jsonld_api a ( opts );
	a.gen_node_map ( o, nodeMap );
	return mk_somap_obj ( nodeMap );
}

qdb cmd_t::toquads ( const strings& args ) {
	return toquads ( load_json ( args ) );
}

qdb cmd_t::toquads ( pobj o ) {
	jsonld::jsonld_api a ( opts );
	rdf_db r ( a );
	auto nodeMap = o;
	for ( auto g : *nodeMap->MAP() ) {
		if ( jsonld::is_rel_iri ( g.first ) ) continue;
		if ( !g.second || !g.second->MAP() ) throw logic_error ( "Expected map in nodemap." );
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	//	dout << "Converting: " << endl << o->toString() << endl;
	//	dout << "Converted: " << endl << r << endl;
	return r;
}

qdb cmd_t::convert ( pobj o ) {
	return toquads ( nodemap ( jsonld::expand ( o, opts ) ) );
}

qdb cmd_t::convert ( const string& s, bool debugprint ) {
	if ( fnamebase ) opts.base = pstr ( string ( "file://" ) + s + "#" );
	if ( debugprint ) dout << " Converting: " << s;
	qdb r = convert ( load_json ( s ) );
	if ( debugprint ) dout << " Converted: " << r << endl;
	return r;
}

void process_flags ( const cmds_t& cmds, strings& args ) {
	strings::iterator it;
	for ( auto x : cmds.second )
		if ( ( it = find ( args.begin(), args.end(), x.first.first ) ) != args.end() ) {
			*x.second = !*x.second;
			args.erase ( it );
		}
}

void print_usage ( const cmds_t& cmds ) {
	dout << endl << "Tau-Chain by http://idni.org" << endl;
	dout << endl << "Usage:" << endl;
	dout << "\ttau help <command>\t\tPrints usage of <command>." << endl;
	dout << "\ttau <command> [<args>]\t\tRun <command> with <args>." << endl;
	dout << endl << "Available commands:" << endl << endl;
	for ( auto c : cmds.first ) dout << '\t' << c.first << '\t' << c.second->desc() << endl;
	dout << endl << "Available flags:" << endl << endl;
	for ( auto c : cmds.second ) dout << '\t' << c.first.first << '\t' << c.first.second << endl;
	dout << endl;
}
