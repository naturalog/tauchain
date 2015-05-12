#include "cli.h"
#include <boost/filesystem.hpp>
#include "parsers.h"
#include "prover.h"
#include <iomanip>

namespace jsonld {

pqlist mk_qlist() {
	return make_shared<qlist>();
}

string node::tostring() {
	stringstream ss;
	if ( _type == IRI ) ss << '<';
	if ( _type == LITERAL ) ss << '\"';
	ss << value;
	if ( _type == LITERAL ) ss << '\"';
	if ( _type == LITERAL && lang.size() ) ss << '@' << lang;
	if ( _type == IRI ) ss << '>';
	return ss.str();
}

pnode mkliteral ( string value, pstring datatype, pstring language ) {
	pnode r = make_shared <node> ( node::LITERAL );
	r->type = "literal";
	r->value = value;
	//	dout<<"mkliteral value: "<<value<<endl;
	r->datatype = datatype ? *datatype : XSD_STRING;
	if ( language ) r->lang = *language;
	return r;
}

pnode mkiri ( string iri ) {
	pnode r = make_shared <node> ( node::IRI );
	r->type = "IRI";
	r->value = iri;
	//	dout<<"mkiri value: "<<iri<<endl;
	return r;
}

pnode mkbnode ( string attribute ) {
	pnode r = make_shared <node> ( node::BNODE );
	r->type = "blank node";
	//	dout<<"mkbnode value: "<<attribute<<endl;
	r->value = attribute;
	return r;
}

const pnode first = mkiri ( RDF_FIRST );
const pnode rest = mkiri ( RDF_REST );
const pnode nil = mkiri ( RDF_NIL );

rdf_db::rdf_db ( jsonld_api& api_ ) :
	qdb(), api ( api_ ) {
	( *this ) [str_default] = mk_qlist();
}

ostream& operator<< ( ostream& o, const qlist& x ) {
	for ( pquad q : x )
		o << q->tostring ( ) << endl;
	return o;
}

ostream& operator<< ( ostream& o, const qdb& q ) {
	for ( auto x : q )
		o /*<< x.first*/ << *x.second;
	return o;
}

string rdf_db::tostring() {
	stringstream s;
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
	if ( has ( rval, "" ) ) {
		rval[str_vocab] = rval[""];
		rval.erase ( "" );
	}
	return rval;
}

void rdf_db::parse_ctx ( pobj contextLike ) {
	pcontext context;
	context = context->parse ( contextLike );
	ssmap prefixes = context->getPrefixes ( true );

	for ( auto x : prefixes ) {
		const string &key = x.first, &val = x.second;
		if ( key == str_vocab ) setNamespace ( "", val );
		else if ( !keyword ( key ) ) setNamespace ( key, val );
	}
}

void rdf_db::graph_to_rdf ( string graph_name, somap& graph ) {
	qlist triples;
	for ( auto y : graph ) { // 4.3
		string id = y.first;
		if ( is_rel_iri ( id ) ) continue;
		psomap node = y.second->MAP();
		for ( auto x : *node ) {
			string property = x.first;
			polist values; // = x.second;
			if ( property == str_type ) { // 4.3.2.1
				values = gettype ( node )->LIST(); // ??
				property = RDF_TYPE; // ??
			} else if ( keyword ( property ) ) continue;
			else if ( startsWith ( property, "_:" ) && !api.opts.produceGeneralizedRdf ) continue;
			else if ( is_rel_iri ( property ) ) continue;
			else values = node->at ( property )->LIST();
			// hack
			//if (!values) values = ((*node)[property] = mk_olist_obj(olist(1, node->at ( property )->clone())))->LIST();

			pnode subj = id.find ( "_:" ) ? mkiri ( id ) : mkbnode ( id );
			pnode pred = startsWith ( property, "_:" ) ?  mkbnode ( property ) : mkiri ( property );

			for ( auto item : *values ) {
				// convert @list to triples
				if ( item->MAP() && haslist ( item->MAP() ) ) {
					polist list = getlist ( item )->LIST();
					pnode last = 0;
					pnode firstBnode = nil;
					if ( list && list->size() ) {
						last = obj_to_rdf ( *list->rbegin() );
						firstBnode = mkbnode ( api.gen_bnode_id() );
					}
					triples.push_back ( make_shared <quad> ( subj, pred, firstBnode, graph_name ) );
					//					trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
					for ( int i = 0; i < ( ( int ) list->size() ) - 1; ++i ) {
						pnode object = obj_to_rdf ( list->at ( i ) );
						triples.push_back ( make_shared <quad> ( firstBnode, first, object, graph_name ) );
						//						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
						pnode restBnode = mkbnode ( api.gen_bnode_id() );
						triples.push_back ( make_shared <quad> ( firstBnode, rest, restBnode, graph_name ) );
						//						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
						firstBnode = restBnode;
					}
					if ( last ) {
						triples.push_back ( make_shared <quad> ( firstBnode, first, last, graph_name ) );
						//						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
						triples.push_back ( make_shared <quad> ( firstBnode, rest, nil, graph_name ) );
						//						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
					}
				} else if ( pnode object = obj_to_rdf ( item ) ) {
					triples.push_back ( make_shared <quad> ( subj, pred, object, graph_name ) );
					//					trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
				}
			}
		}
	}
	//	orig insertion:
	if ( find ( graph_name ) == end() ) ( *this ) [graph_name] = make_shared <qlist> ( triples );
	else at ( graph_name )->insert ( at ( graph_name )->end() , triples.begin(), triples.end() );
	//	alternative insertion:
	//	for (auto t : triples) {
	//		if (find(t.graph->value) == end())
	//	}
}

pnode rdf_db::obj_to_rdf ( pobj item ) {
	if ( isvalue ( item ) ) {
		pobj value = getvalue ( item ), datatype = sgettype ( item );
		if ( !value )
			return 0;
		if ( value->BOOL() || value->INT() || value->UINT() || value->DOUBLE() ) {
			if ( value->BOOL() ) return mkliteral ( *value->BOOL() ? "(true)" : "(false)", pstr ( datatype ? *datatype->STR() : XSD_BOOLEAN ), 0 );
			else if ( value->DOUBLE() || ( datatype && XSD_DOUBLE == *datatype->STR() ) )
				return mkliteral ( tostr ( *value->DOUBLE() ), pstr ( datatype ? *datatype->STR() : XSD_DOUBLE ), 0 );
			else return mkliteral ( value->INT() ?  tostr ( *value->INT() ) : tostr ( *value->UINT() ), pstr ( datatype ? *datatype->STR() : XSD_INTEGER ), 0 );
		} else if ( haslang ( item->MAP() ) )
			return mkliteral ( *value->STR(), pstr ( datatype ? *datatype->STR() : RDF_LANGSTRING ), getlang ( item )->STR() );
		else return mkliteral ( *value->STR(), pstr ( datatype ? *datatype->STR() : XSD_STRING ), 0 );
	}
	// convert string/node object to RDF
	else {
		string id;
		if ( item->MAP() ) {
			id = *getid ( item )->STR();
			if ( is_rel_iri ( id ) ) return 0;
		} else id = *item->STR();
		return id.find ( "_:" ) ? mkiri ( id ) : mkbnode ( id );
	}
}

}

quad::quad ( string subj, string pred, pnode object, string graph ) :
	quad ( startsWith ( subj, "_:" ) ? mkbnode ( subj ) : mkiri ( subj ), mkiri ( pred ), object, graph ) {
}

quad::quad ( string subj, string pred, string object, string graph ) :
	quad ( subj, pred,
	       startsWith ( object, "_:" ) ? mkbnode ( object ) : mkiri ( object ),
	       graph ) {}

quad::quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph ) :
	quad ( subj, pred, mkliteral ( value, datatype, language ), graph ) { }

quad::quad ( pnode subj, pnode pred, pnode object, string graph ) :
	quad_base ( subj, pred, object, startsWith ( graph, "_:" ) ? mkbnode ( graph ) : mkiri ( graph ) ) { }

string quad::tostring ( ) {
	stringstream ss;
	bool _shorten = shorten;
	auto f = [_shorten] ( pnode n ) {
		if ( n ) {
			string s = n->tostring();
			if ( !shorten ) return s;
			if ( s.find ( "#" ) == string::npos ) return s;
			return s.substr ( s.find ( "#" ), s.size() - s.find ( "#" ) );
		}
		return string ( "<>" );
	};
	ss <<setw(10)<< f ( subj ) << setw(10)<<' ' << f ( pred ) <<setw(10)<<' ' << f ( object );
	if ( graph->value != str_default ) ss <<setw(10)<<' ' << f ( graph );
	ss << " .";
	return ss.str();
}

#include <boost/algorithm/string.hpp>
using namespace std;
using namespace boost::algorithm;

namespace jsonld {

qdb readqdb ( istream& is) {
	string s,p,o,c;
	bool quotes = false;
	const char quote = '\"';
	int pos = -1;
	char ch;
	pquad pq;
	qdb q;
	auto tr = [](string s){
		trim_if(s, is_any_of(" <>"));
		return s;
	};
	while (is.get(ch)) {
		if (ch == quote) {
			quotes = !quotes;
			continue;
		}
		if (!quotes) {
			if (isspace(ch) || ch == '\r' || ch == '\n') {
				if (pos != -1)
					++pos;
				continue;
			}
			if (pos == -1)
				pos = 0;
			if (ch == '.') {
				if (p == "=>")
					p = implication;
				if (pos == 0 && s == "fin")
					return q;
				if (pos == 2)
					pq = make_shared<quad>(tr(s), tr(p), tr(o), c = "@default");
				else if (pos == 3) {
					c = tr(c);
					if (!c.size())
						c = "@default";
					pq = make_shared<quad>(tr(s), tr(p), tr(o), tr(c));
				}
//				dout << pq->tostring() << endl;
#ifdef IRC				
				sleep(1);
#endif				
				if (q.find(c) == q.end())
					q[c] = make_shared<qlist>();
				q[c]->push_back(pq);
				pos = -1;
				s=p=o=c="";
			} else
				switch (pos) {
				case 0: s += ch; break;
				case 1: p += ch; break;
				case 2: o += ch; break;
				case 3: c += ch; break;
				default: break;
				}
		}
	}
	return q;
}
}

string expand_cmd::desc() const {
	return "Run expansion algorithm http://www.w3.org/TR/json-ld-api/#expansion-algorithms including all dependant algorithms.";
}
string expand_cmd::help() const {
	stringstream ss ( "Usage:" );
	ss << endl << "\ttau expand [JSON-LD input filename]";
	ss << endl << "\ttau expand [JSON-LD input filename] [JSON-LD output to compare to]";
	ss << endl << "If input filename is unspecified, reads from stdin." << endl;
	return ss.str();
}
int expand_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 4 ) {
		dout << help();
		return 1;
	}
	pobj e;
	dout << ( e = jsonld::expand ( load_json ( args ) ) )->toString() << endl;
	if ( args.size() == 3 ) return 0;
	boost::filesystem::path tmpdir = boost::filesystem::temp_directory_path();
	const string tmpfile_template =  tmpdir.string() + "/XXXXXX";
	std::unique_ptr<char> fn1 ( strdup ( tmpfile_template.c_str() ) );
	std::unique_ptr<char> fn2 ( strdup ( tmpfile_template.c_str() ) );
	int fd1 = mkstemp ( fn1.get() );
	int fd2 = mkstemp ( fn2.get() );

	ofstream os1 ( fn1.get() ), os2 ( fn2.get() );
	os1 << json_spirit::write_string ( jsonld::convert ( e ), json_spirit::pretty_print | json_spirit::single_line_arrays ) << std::endl;
	os2 << json_spirit::write_string ( jsonld::convert ( load_json ( args[3] ) ), json_spirit::pretty_print | json_spirit::single_line_arrays ) << std::endl;
	os1.close();
	os2.close();
	string c = string ( "diff " ) + fn1.get() + string ( " " ) + fn2.get();
	if ( system ( c.c_str() ) ) derr << "command " << c << " failed." << endl;
	close ( fd1 );
	close ( fd2 );

	return 0;
}


string convert_cmd::desc() const {
	return "Convert JSON-LD to quads including all dependent algorithms.";
}
string convert_cmd::help() const {
	stringstream ss ( "Usage: tau expand [JSON-LD input filename]" );
	ss << endl << "If input filename is unspecified, reads from stdin." << endl;
	return ss.str();
}
int convert_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 3 ) {
		dout << help();
		return 1;
	}
	try {
		dout << std::setw(20) << convert ( args[2] ) << endl;
		return 0;
	} catch ( exception& ex ) {
		derr << ex.what() << endl;
		return 1;
	}
}

string toquads_cmd::desc() const {
	return "Run JSON-LD->RDF algorithm http://www.w3.org/TR/json-ld-api/#deserialize-json-ld-to-rdf-algorithm of already-expanded input.";
}
string toquads_cmd::help() const {
	stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
	ss << endl << "If input filename is unspecified, reads from stdin." << endl;
	ss << "Note that input has to be expanded first, so you might want to pipe it with 'tau expand' command." << endl;
	return ss.str();
}
int toquads_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 3 ) {
		dout << help();
		return 1;
	}
	try {
		dout << toquads ( args ) << endl;
	} catch ( exception& ex ) {
		derr << ex.what() << endl;
		return 1;
	}
	return 0;
}

string nodemap_cmd::desc() const {
	return "Run JSON-LD node map generation algorithm.";
}
string nodemap_cmd::help() const {
	stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
	ss << endl << "If input filename is unspecified, reads from stdin." << endl;
	return ss.str();
}
int nodemap_cmd::operator() ( const strings& args ) {
	if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 3 ) {
		dout << help();
		return 1;
	}
	try {
		dout << nodemap ( args )->toString() << endl;
	} catch ( exception& ex ) {
		derr << ex.what() << endl;
		return 1;
	}
	return 0;
}
