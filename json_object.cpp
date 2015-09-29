#include "json_object.h"


namespace old{


#ifdef JSON



#ifdef DEBUG
void bt() {
/*	void *trace[16];
	char **messages = 0;
	int i, trace_size = 0;
	trace_size = backtrace ( trace, 16 );
	trace[1] = ( void * ) __builtin_return_address ( 0 );
	messages = backtrace_symbols ( trace, trace_size );
	printf ( "[bt] Execution path:\n" );
	for ( i = 1; i < trace_size; ++i ) {
		printf ( "[bt] #%d %s\n", i, messages[i] );
		size_t p = 0;
		while ( messages[i][p] != '(' && messages[i][p] != ' '
		        && messages[i][p] != 0 )
			++p;
		char syscom[256];
		sprintf ( syscom, "addr2line %p -e %.*s", trace[i], ( int ) p, messages[i] );
		system ( syscom );
	}*/
}

void dopause() {
/*	std::clog << "press any key to continue, b for backtrace, or a to always show backtrace, or c to stop pausing...";
	char ch = getchar();
	if ( ch == 'b' || ( autobt = ( ch == 'a' ) ) ) bt();
	else if ( ch == 'c' ) autobt = _pause = false;
*/}


#else
void bt() {}
void dopause() {}
#define trace(x)
#endif

size_t obj::size() {
	if ( LIST() ) return LIST()->size();
	if ( MAP() ) return MAP()->size();
	return 1;
}

bool obj::STR ( const string& x ) {
	auto y = STR();
	return y && ( *y == x );
}

std::shared_ptr<obj> obj::MAP ( const string& k ) {
	auto y = MAP();
	if ( !y ) return 0;
	somap::iterator it = y->find ( k );
	return it == y->end() ? 0 : it->second;
}

bool obj::map_and_has ( const string& k ) {
	psomap m = MAP();
	return m && m->find ( k ) != m->end();
}

bool obj::map_and_has_null ( const string& k ) {
	psomap m = MAP();
	if ( !m ) return false;
	auto it = m->find ( k );
	return ( it != m->end() ) && ( !it->second || it->second->Null() );
}

pstring_obj mk_str_obj() {
	return std::make_shared<string_obj>();
}

psomap_obj mk_somap_obj() {
	return std::make_shared<somap_obj>();
}

polist_obj mk_olist_obj() {
	return std::make_shared<olist_obj>();
}

polist mk_olist() {
	return std::make_shared<olist>();
}

bool has ( const defined_t& c, const string& k ) {
	return c.find ( k ) != c.end();
}

bool has ( pdefined_t c, const string& k ) {
	return c && has ( *c, k );
}

bool has ( const somap& c, const string& k ) {
	#ifdef VERBOSE
	trace ( "query for key " << k << "form object: " << std::endl << mk_somap_obj ( c )->toString() << std::endl );
	#endif
	return c.find ( k ) != c.end();
}

bool has ( psomap c, const string& k ) {
	return c && has ( *c, k );
}

bool has ( psomap c, pstring k ) {
	return k && has ( c, *k );
}

bool has ( pobj o, string s ) {
	return o && o->MAP() && has ( o->MAP(), s );
}

bool has ( pobj o, pstring s ) {
	return s && has ( o, *s );
}



#endif
pstring pstr ( const wchar_t* s ) { return s ? pstr ( string ( s ) ) : 0; }


}