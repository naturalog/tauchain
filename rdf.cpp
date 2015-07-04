#include "cli.h"
#include <boost/filesystem.hpp>
#include "prover.h"
#include "jsonld.h"
#include <iomanip>
#include "misc.h"

pqlist mk_qlist() {
	return make_shared<qlist>();
}

string node::tostring() const {
	std::wstringstream ss;
//	if ( _type == IRI && (*value)[0] != L'?' ) ss << L'<';
	if ( _type == LITERAL ) ss << L'\"';
	ss << *value;
	if ( _type == LITERAL ) {
		ss << L'\"';
		if ( datatype && datatype->size() ) ss << "^^" << *datatype;
		if ( lang && lang->size() ) ss << L'@' << *lang;
	}
//	if ( _type == IRI && (*value)[0] != L'?' ) ss << L'>';
	return ss.str();
}

pnode mkliteral ( pstring value, pstring datatype, pstring language ) {
	setproc(L"mkliteral");
//	TRACE(dout << *value << endl);
	node r ( node::LITERAL );
	r.value = value;
	if (!datatype) r.datatype = XSD_STRING;
	else {
		const string& dt = *datatype;
		if (dt == L"XSD_STRING") r.datatype = XSD_STRING;
		else if (dt == L"XSD_INTEGER") r.datatype = XSD_INTEGER;
		else if (dt == L"XSD_DOUBLE") r.datatype = XSD_DOUBLE;
		else if (dt == L"XSD_BOOLEAN") r.datatype = XSD_BOOLEAN;
		else if (dt == L"XSD_FLOAT") r.datatype = XSD_FLOAT;
		else if (dt == L"XSD_DECIMAL") r.datatype = XSD_DECIMAL;
		else if (dt == L"XSD_ANYTYPE") r.datatype = XSD_ANYTYPE;
		else if (dt == L"XSD_ANYURI") r.datatype = XSD_ANYURI;
//		else if (dt == L"XSD_PTR") r.datatype = XSD_PTR;
		else r.datatype = datatype;
	}
	if ( language ) r.lang = language;
	auto it =  dict.nodes.find(*r.value);
	if (it != dict.nodes.end()) return it->second;
	pnode pr = make_shared<node>(r); 
	dict.set(pr);
	return dict.nodes[*r.value] = pr;
}

pnode mkiri ( pstring iri ) {
	setproc(L"mkiri");
//	TRACE(dout << *iri << endl);
	node r ( node::IRI );
	r.value = iri;
	auto it =  dict.nodes.find(*r.value);
	if (it != dict.nodes.end()) return it->second;
	pnode pr = make_shared<node>(r); 
	dict.set(pr);
	return dict.nodes[*r.value] = pr;
}

pnode mkbnode ( pstring attribute ) {
	setproc(L"mkbnode");
//	TRACE(dout << *attribute << endl);
	node r ( node::BNODE );
	r.value = attribute;
	auto it =  dict.nodes.find(*r.value);
	if (it != dict.nodes.end()) {
//		TRACE(dout<<" returned existing node" << endl);
		return it->second;
	}
	pnode pr = make_shared<node>(r); 
	dict.set(pr);
	return dict.nodes[*r.value] = pr;
}

rdf_db::rdf_db ( jsonld_api& api_ ) :
	qdb(), api ( api_ ) {
	first[str_default] = mk_qlist();
}

std::wostream& operator<< ( std::wostream& o, const qlist& x ) {
	for ( pquad q : x )
		o << q->tostring ( ) << std::endl;
	return o;
}

std::wostream& operator<< ( std::wostream& o, const qdb& q ) {
	for ( auto x : q.first )
		o /*<< x.first*/ << *x.second;
	return o;
}

string rdf_db::tostring() {
	std::wstringstream s;
	s << *this;
	return s.str();
}

void rdf_db::setNamespace ( string ns, string prefix ) {
	context[ns] = prefix;
}

string rdf_db::getNamespace ( string ns ) {
	return context[ns];
}

void rdf_db::clearNamespaces() {
	context.clear();
}

ssmap& rdf_db::getNamespaces() {
	return context;
}

somap rdf_db::getContext() {
	somap rval;
	for ( auto x : context )
		rval[x.first] = mk_str_obj ( x.second );
	if ( has ( rval, L"" ) ) {
		rval[str_vocab] = rval[L""];
		rval.erase ( L"" );
	}
	return rval;
}

void rdf_db::parse_ctx ( pobj contextLike ) {
	pcontext context;
	context = context->parse ( contextLike );
	ssmap prefixes = context->getPrefixes ( true );

	for ( auto x : prefixes ) {
		const string &key = x.first, &val = x.second;
		if ( key == str_vocab ) setNamespace ( L"", val );
		else if ( !keyword ( key ) ) setNamespace ( key, val );
	}
}

void rdf_db::graph_to_rdf ( string graph_name, somap& graph ) {
	qlist triples;
	{pnode first = mkiri ( RDF_FIRST );
	pnode rest = mkiri ( RDF_REST );
	pnode nil = mkiri ( RDF_NIL );
	for ( auto y : graph ) { // 4.3
		pstring id = pstr(y.first);
		if ( is_rel_iri ( id ) ) continue;
		psomap node = y.second->MAP();
		for ( auto x : *node ) {
			pstring property = pstr(x.first);
			polist values;
			if ( *property == str_type ) { // 4.3.2.1
				values = gettype ( node )->LIST(); // ??
				property = RDF_TYPE; // ??
			} else if ( keyword ( property ) ) continue;
			else if ( startsWith ( property, L"_:" ) && !api.opts.produceGeneralizedRdf ) continue;
			else if ( is_rel_iri ( property ) ) continue;
			else values = node->at ( *property )->LIST();

			pnode subj = id->find ( L"_:" ) ? mkiri ( id ) : mkbnode ( id );
			pnode pred = startsWith ( property, L"_:" ) ?  mkbnode ( property ) : mkiri ( property );

			for ( auto item : *values ) {
				if ( item->MAP() && haslist ( item->MAP() ) ) {
					polist list = getlist ( item )->LIST();
					pnode last = 0;
					pnode firstBnode = nil;
					if ( list && list->size() ) {
						last = obj_to_rdf ( *list->rbegin() );
						firstBnode = mkbnode ( api.gen_bnode_id() );
					}
					triples.push_back ( make_shared <quad> ( subj, pred, firstBnode, graph_name ) );
					for ( int i = 0; i < ( ( int ) list->size() ) - 1; ++i ) {
						pnode object = obj_to_rdf ( list->at ( i ) );
						triples.push_back ( make_shared <quad> ( firstBnode, first, object, graph_name ) );
						second[*firstBnode->value].push_back(firstBnode);
						pnode restBnode = mkbnode ( api.gen_bnode_id() );
						triples.push_back ( make_shared <quad> ( firstBnode, rest, restBnode, graph_name ) );
						firstBnode = restBnode;
					}
					if ( last ) {
						triples.push_back ( make_shared <quad> ( firstBnode, first, last, graph_name ) );
						triples.push_back ( make_shared <quad> ( firstBnode, rest, nil, graph_name ) );
					}
				} else if ( pnode object = obj_to_rdf ( item ) ) 
					triples.push_back ( make_shared <quad> ( subj, pred, object, graph_name ) );
			}
		}
	}}
	if ( first.find ( graph_name ) == first.end() ) first[graph_name] = make_shared <qlist> ( triples );
	else first[graph_name]->insert ( first[graph_name]->end() , triples.begin(), triples.end() );
}

pnode rdf_db::obj_to_rdf ( pobj item ) {
	if ( isvalue ( item ) ) {
		pobj value = getvalue ( item ), datatype = sgettype ( item );
		if ( !value )
			return 0;
		if ( value->BOOL() || value->INT() || value->UINT() || value->DOUBLE() ) {
			if ( value->BOOL() ) 
				return mkliteral ( pstr(*value->BOOL() ? L"true" : L"false"), datatype ? pstr(*datatype->STR()) : XSD_BOOLEAN, 0 );
			else if ( value->DOUBLE() || ( datatype && *XSD_DOUBLE == *datatype->STR() ) )
				return mkliteral ( tostr ( *value->DOUBLE() ), datatype ? pstr( *datatype->STR() ) : XSD_DOUBLE, 0 );
			else 
				return mkliteral ( 
					value->INT() ?  tostr ( *value->INT() ) : tostr ( *value->UINT() ), 
					datatype ? pstr(*datatype->STR()) : XSD_INTEGER, 0 );
		} else if ( haslang ( item->MAP() ) )
			return mkliteral ( pstr(*value->STR()), pstr ( datatype ? *datatype->STR() : RDF_LANGSTRING ), getlang ( item )->STR() );
		else 
			return mkliteral ( pstr(*value->STR()), datatype ? pstr(*datatype->STR()) : XSD_STRING, 0 );
	}
	// convert string/node object to RDF
	else {
		string id;
		if ( item->MAP() ) {
			id = *getid ( item )->STR();
			if ( is_rel_iri ( id ) ) return 0;
		} else id = *item->STR();
		return id.find ( L"_:" ) ? mkiri ( pstr(id) ) : mkbnode ( pstr(id) );
	}
}

quad::quad ( string subj, string pred, pnode object, string graph ) :
	quad ( startsWith ( subj, L"_:" ) ? mkbnode ( pstr(subj) ) : mkiri ( pstr(subj) ), mkiri ( pstr(pred) ), object, graph ) {
}

quad::quad ( string subj, string pred, string object, string graph ) :
	quad ( subj, pred,
	       startsWith ( object, L"_:" ) ? mkbnode ( pstr(object) ) : mkiri ( pstr(object) ),
	       graph ) {}

quad::quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph ) :
	quad ( subj, pred, mkliteral ( pstr(value), datatype, language ), graph ) { }

quad::quad ( pnode s, pnode p, pnode o, string c ) :
	subj(s), pred(p), object(o), graph(startsWith ( c, L"_:" ) ? mkbnode ( pstr(c) ) : mkiri ( pstr(c) ) ) {
		setproc(L"quad::ctor");
		TRACE(dout<<tostring()<<endl);
	}

string quad::tostring ( ) const {
	std::wstringstream ss;
	bool _shorten = shorten;
	auto f = [_shorten] ( pnode n ) {
		if ( n ) {
			string s = n->tostring();
			if ( !shorten ) return s;
			if ( s.find ( L"#" ) == string::npos ) return s;
			return s.substr ( s.find ( L"#" ), s.size() - s.find ( L"#" ) );
		}
		return string ( L"<>" );
	};
	ss << f ( subj ) << L' ' << f ( pred ) << L' ' << f ( object );
	if ( *graph->value != str_default ) ss << L' ' << f ( graph );
//	ss <<setw(10)<< f ( subj ) << setw(10)<<' ' << f ( pred ) <<setw(10)<<' ' << f ( object );
//	if ( graph->value != str_default ) ss <<setw(10)<<' ' << f ( graph );
	ss << L" .";
	return ss.str();
}

#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

qdb readqdb ( std::wistream& is) {
	string s, c;
	quad q;
	qdb r;
	nqparser p;
	std::wstringstream ss;
	while (getline(is, s)) {
//		trim(s);
		if (s[0] == L'#') continue;
		ss << s << ' ';
	}
	auto rr = p(ss.str().c_str());
	r.second = rr.second;
	for (quad q : rr.first) {
		c = *q.graph->value;
		if (r.first.find(c) == r.first.end()) r.first[c] = make_shared<qlist>();
		r.first[c]->push_back(make_shared<quad>(q));
	}
	return r;
}

std::string expand_cmd::desc() const {
	return "Run expansion algorithm http://www.w3.org/TR/json-ld-api/#expansion-algorithms including all dependant algorithms.";
}

std::string expand_cmd::help() const {
	std::stringstream ss ( "Usage:" );
	ss << std::endl << "\ttau expand [JSON-LD input filename]";
	ss << std::endl << "\ttau expand [JSON-LD input filename] [JSON-LD output to compare to]";
	ss << std::endl << "If input filename is unspecified, reads from stdin." << std::endl;
	return ss.str();
}

int expand_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == L"help" ) || args.size() > 4 ) {
		dout << ws(help());
		return 1;
	}
	pobj e;
	dout << ( e = expand ( load_json ( args ) ) )->toString() << std::endl;
	if ( args.size() == 3 ) return 0;
	boost::filesystem::path tmpdir = boost::filesystem::temp_directory_path();
	const std::string tmpfile_template =  tmpdir.string() + "/XXXXXX";
	std::unique_ptr<char> fn1 ( strdup ( tmpfile_template.c_str() ) );
	std::unique_ptr<char> fn2 ( strdup ( tmpfile_template.c_str() ) );
	int fd1 = mkstemp ( fn1.get() );
	int fd2 = mkstemp ( fn2.get() );

	std::wofstream os1 ( fn1.get() ), os2 ( fn2.get() );
	os1 << json_spirit::write_string ( ::convert ( e ), json_spirit::pretty_print | json_spirit::single_line_arrays ) << std::endl;
	os2 << json_spirit::write_string ( ::convert ( load_json ( args[3] ) ), json_spirit::pretty_print | json_spirit::single_line_arrays ) << std::endl;
	os1.close();
	os2.close();
	string c = string ( L"diff " ) + ws(fn1.get()) + string ( L" " ) + ws(fn2.get());
	if ( system ( ws(c).c_str() ) ) derr << L"command " << c << L" failed." << std::endl;
	close ( fd1 );
	close ( fd2 );

	return 0;
}


std::string convert_cmd::desc() const {
	return "Convert JSON-LD to quads including all dependent algorithms.";
}

std::string convert_cmd::help() const {
	std::stringstream ss ( "Usage: tau expand [JSON-LD input filename]" );
	ss << std::endl << "If input filename is unspecified, reads from stdin." << std::endl;
	return ss.str();
}

int convert_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == L"help" ) || args.size() > 3 ) {
		dout << ws(help());
		return 1;
	}
	try {
		dout << std::setw(20) << convert ( args[2] ) << std::endl;
		return 0;
	} catch ( std::exception& ex ) {
		derr << ex.what() << std::endl;
		return 1;
	}
}

std::string toquads_cmd::desc() const {
	return "Run JSON-LD->RDF algorithm http://www.w3.org/TR/json-ld-api/#deserialize-json-ld-to-rdf-algorithm of already-expanded input.";
}

std::string toquads_cmd::help() const {
	std::stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
	ss << std::endl << "If input filename is unspecified, reads from stdin." << std::endl;
	ss << "Note that input has to be expanded first, so you might want to pipe it with 'tau expand' command." << std::endl;
	return ss.str();
}

int toquads_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == L"help" ) || args.size() > 3 ) {
		dout << ws(help());
		return 1;
	}
	try {
		dout << toquads ( args ) << std::endl;
	} catch ( std::exception& ex ) {
		derr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}

std::string nodemap_cmd::desc() const {
	return "Run JSON-LD node map generation algorithm.";
}

std::string nodemap_cmd::help() const {
	std::stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
	ss << std::endl << "If input filename is unspecified, reads from stdin." << std::endl;
	return ss.str();
}

int nodemap_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == L"help" ) || args.size() > 3 ) {
		dout << ws(help());
		return 1;
	}
	try {
		dout << nodemap ( args )->toString() << std::endl;
	} catch ( std::exception& ex ) {
		derr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
