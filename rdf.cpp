#include "jsonld_tau.h"
//#include <boost/filesystem.hpp>
#include "prover.h"
#include "jsonld.h"
#include <iomanip>
#include "misc.h"
#include <boost/algorithm/string.hpp>



pqlist mk_qlist() {
	return make_shared<qlist>();
}

string node::tostring() const {
	std::stringstream ss;
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


pnode set_dict(node r){	
	//If the dictionary already contains a node with this name,
	//return that one.
	//#dict.nodes : std::map<string,pnode>;
	auto it = dict.nodes.find(r.tostring());
	if (it != dict.nodes.end()) {
		assert(it->second->_type == r._type);
		return it->second;
	}	
	//Otherwise put it into the dictionary and return
	//a shared pointer to it. Why is putting it into
	//dict.nodes not part of dict.set?
	pnode pr = make_shared<node>(r);
	dict.set(pr);
	return dict.nodes[r.tostring()] = pr;
}

//Could just make all these constructors for the node class:
//	mkliteral, mkiri, and mkbnode, that is.
pnode mkliteral ( pstring value, pstring datatype, pstring language ) {
	setproc("mkliteral");
//	TRACE(dout << *value << endl);
	if (!value) throw std::runtime_error("mkliteral: null value given");


	node r ( node::LITERAL );
	r.value = value;
	if (!datatype) r.datatype = XSD_STRING;
	else {
		const string& dt = *datatype;
		//--do we need these cases? looks like we're usually
		//just sending it the global XSD_* anyway
//well the point of those chained ifs is to point it to one shared string instead of leave each node carry its own datatype string
		//--in any case, maybe better to put these in an 
		//associative array
//sure
		if (dt == "XSD_STRING") r.datatype = XSD_STRING;
		else if (dt == "XSD_INTEGER") r.datatype = XSD_INTEGER;
		else if (dt == "XSD_DOUBLE") r.datatype = XSD_DOUBLE;
		else if (dt == "XSD_BOOLEAN") r.datatype = XSD_BOOLEAN;
		else if (dt == "XSD_FLOAT") r.datatype = XSD_FLOAT;
		else if (dt == "XSD_DECIMA") r.datatype = XSD_DECIMAL;
		else if (dt == "XSD_ANYTYPE") r.datatype = XSD_ANYTYPE;
		else if (dt == "XSD_ANYURI") r.datatype = XSD_ANYURI;
		else r.datatype = datatype;
	}

	if ( language ) r.lang = language;

	return set_dict(r);
}



pnode mkiri ( pstring iri ) {
	setproc("mkiri");

	if (!iri) throw std::runtime_error("mkiri: null iri given");
//	TRACE(dout << *iri << endl);
	node r ( node::IRI );
	r.value = iri;
	return set_dict(r);
}

/*
//Should use this one instead:


pnode mkiri ( pstring iri ) {
	setproc("mkiri");

	if(!iri) throw std::runtime_error("mkiri: null iri given");
	auto it = dict.nodes.find(*iri);
	if(it != dict.nodes.end()){
		assert(it->second->_type == node::IRI);
		return it->second;
	}
	node r ( node::IRI);
	r.value = iri;
	pnode pr = make_shared<node>(r);
	dict.set(pr);
	return dict.nodes[r.tostring()] = pr;
}

*/



pnode mkbnode ( pstring attribute ) {
	setproc("mkbnode");
	if (!attribute) throw std::runtime_error("mkbnode: null value given");
//	TRACE(dout << *attribute << endl);
	node r ( node::BNODE );
	r.value = attribute;
	return set_dict(r);
}






std::ostream& operator<< ( std::ostream& o, const qlist& x ) {
	for ( pquad q : x )
		o << q->tostring ( ) << std::endl;
	return o;
}

std::ostream& operator<< ( std::ostream& o, const std::list<pnode>& x ) {
	for ( pnode n : x )
		o << n->tostring ( ) << std::endl;
	return o;
}

std::ostream& operator<< ( std::ostream& o, const qdb& q ) {
	for ( auto x : q.first )
		o << x.first << ": " << std::endl << *x.second << std::endl;
	for ( auto x : q.second )
		o << x.first << ": " << std::endl << x.second << std::endl;
	return o;
}




#ifdef JSON
rdf_db::rdf_db ( jsonld_api& api_ ) :
	qdb(), api ( api_ ) {
	first[str_default] = mk_qlist();
}


string rdf_db::tostring() {
	std::stringstream s;
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


void rdf_db::graph_to_rdf(string graph_name, somap &graph)
{
	qlist triples;
	{
		pnode first = mkiri(RDF_FIRST);
		pnode rest = mkiri(RDF_REST);
		pnode nil = mkiri(RDF_NIL);
		for (auto y : graph) { // 4.3
			pstring id = pstr(y.first);
			if (is_rel_iri(id)) continue;
			psomap node = y.second->MAP();
			for (auto x : *node) {
				pstring property = pstr(x.first);
				polist values;
				if (*property == str_type) { // 4.3.2.1
					values = gettype(node)->LIST(); // ??
					property = RDF_TYPE; // ??
				} else if (keyword(property)) continue;
				else if (startsWith(property, "_:") && !api.opts.produceGeneralizedRdf) continue;
				else if (is_rel_iri(property)) continue;
				else values = node->at(*property)->LIST();

				pnode subj = id->find("_:") ? mkiri(id) : mkbnode(id);
				pnode pred = startsWith(property, "_:") ? mkbnode(property) : mkiri(property);

				for (auto item : *values) {
					if (item->MAP() && haslist(item->MAP())) {
						polist list = getlist(item)->LIST();
						pnode last = 0;
						pnode firstBnode = nil, head;
						if (list && list->size()) {
							last = obj_to_rdf(*list->rbegin());
							head = mkbnode(gen_bnode_id());
						}
						triples.push_back(make_shared<quad>(subj, pred, firstBnode, graph_name));
						for (int i = 0; i < ((int) list->size()) - 1; ++i) {
							pnode object = obj_to_rdf(list->at(i));
							triples.push_back(make_shared<quad>(firstBnode, first, object, graph_name));
							second[*head->value].push_back(object);
							pnode restBnode = mkbnode(gen_bnode_id());
							triples.push_back(make_shared<quad>(firstBnode, rest, restBnode, graph_name));
							firstBnode = restBnode;
						}
						if (last) {
							triples.push_back(make_shared<quad>(firstBnode, first, last, graph_name));
							triples.push_back(make_shared<quad>(firstBnode, rest, nil, graph_name));
						}
					} else if (pnode object = obj_to_rdf(item))
						triples.push_back(make_shared<quad>(subj, pred, object, graph_name));
				}
			}
		}
	}
	if (first.find(graph_name) == first.end()) first[graph_name] = make_shared<qlist>(triples);
	else first[graph_name]->insert(first[graph_name]->end(), triples.begin(), triples.end());
}








pnode rdf_db::obj_to_rdf ( pobj item ) {
	if ( isvalue ( item ) ) {
		pobj value = getvalue ( item ), datatype = sgettype ( item );
		if ( !value )
			return 0;
		if ( value->BOOL() || value->INT() || value->UINT() || value->DOUBLE() ) {
			if ( value->BOOL() ) 
				return mkliteral ( pstr(*value->BOOL() ? "true" : "false"), datatype ? pstr(*datatype->STR()) : XSD_BOOLEAN, 0 );
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
	else {
		string id;
		if ( item->MAP() ) {
			id = *getid ( item )->STR();
			if ( is_rel_iri ( id ) ) return 0;
		} else id = *item->STR();
		return id.find ( "_:" ) ? mkiri ( pstr(id) ) : mkbnode ( pstr(id) );
	}
}
#endif






quad::quad ( string subj, string pred, pnode object, string graph ) :
	quad ( startsWith ( subj, "_:" ) ? mkbnode ( pstr(subj) ) : mkiri ( pstr(subj) ), mkiri ( pstr(pred) ), object, graph ) {
}



quad::quad ( string subj, string pred, string object, string graph ) :
	quad ( subj, pred,
	       startsWith ( object, "_:" ) ? mkbnode ( pstr(object) ) : mkiri ( pstr(object) ),
	       graph ) {}



quad::quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph ) :
	quad ( subj, pred, mkliteral ( pstr(value), datatype, language ), graph ) { }




quad::quad ( pnode s, pnode p, pnode o, string c ) :
	subj(s), pred(p), object(o), graph(startsWith ( c, "_:" ) ? mkbnode ( pstr(c) ) : mkiri ( pstr(c) ) ) {
		setproc("quad::ctor");
		TRACE(dout<<tostring()<<endl);
	}




quad::quad ( pnode s, pnode p, pnode o, pnode c ) : quad(s, p, o, *c->value){}





string quad::tostring ( ) const {
	std::stringstream ss;
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
	ss << f ( subj ) << L' ' << f ( pred ) << L' ' << f ( object );
	if ( *graph->value != str_default ) ss << L' ' << f ( graph );
	ss << " .";
	return ss.str();
}







using namespace boost::algorithm;
/*
#ifndef NOPARSER
int readqdb (qdb& r, std::istream& is) {
	return parse_nq(r,is);
}
#endif
*/

