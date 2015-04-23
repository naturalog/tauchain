#include "cli.h"

int main ( int argc, char** argv ) {
	strings args;
	for ( int n = 0; n < argc; ++n ) args.push_back ( argv[n] );
	if ( argc == 1 || ( cmds.find ( argv[1] ) == cmds.end() && args[1] != "help" ) ) {
		cout << endl << "Tau-Chain by http://idni.org" << endl;
		cout << endl << "Usage:" << endl;
		cout << "\ttau help <command>\t\tPrints usage of <command>." << endl;
		cout << "\ttau <command> [<args>]\t\tRun <command> with <args>." << endl;
		cout << endl << "Availiable commands:" << endl << endl;
		for ( auto c : cmds ) cout << '\t' << c.first << '\t' << c.second->desc() << endl;
		return 1;
	}
	if ( args[1] == "help" && argc == 3 ) {
		cout << cmds[string ( argv[2] )]->help();
		return 0;
	}
	return ( *cmds[argv[1]] ) ( args );

	//	print_evidence(prove ( *kb["@default"], *query["@default"] ));

	return 0;
}
