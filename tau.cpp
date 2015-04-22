#include "reasoner.h"

int main ( int argc, char** argv ) {
	if ( argc == 1 ) funtest();
	if ( argc != 2 && argc != 3 ) {
		cout << "Usage:" << endl << "\t" << argv[0] << " [<JSON-LD kb file> [JSON-LD query file]]" << endl;
		return 1;
	}

	cout << "input:" << argv[1] << endl;
	auto kb = use_nquads ? load_nq ( argv[1] ) : jsonld::load_jsonld ( argv[1], true );
	cout << kb.tostring() << endl;

	if ( argc == 2 ) return 0;

	auto query = use_nquads ? load_nq ( argv[2] ) : jsonld::load_jsonld ( argv[2], true );
	auto evidence = prove ( *kb["@default"], *query["@default"] );
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
		cout << endl << "---" << endl;
	}
	return 0;
}
