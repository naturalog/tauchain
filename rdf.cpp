#include "jsonld.h"

namespace jsonld {

string node::tostring() {
	stringstream ss;
	if ( _type == IRI || _type == BNODE ) ss << '<';
	if ( _type == LITERAL ) ss << '\"';
	ss << value;
	if ( _type == LITERAL ) ss << '\"';
	if ( _type == LITERAL && lang.size() ) ss << '@' << lang;
	if ( _type == IRI || _type == BNODE ) ss << '>';
	return ss.str();
}

pnode mkliteral ( string value, pstring datatype, pstring language ) {
	pnode r = make_shared <node> ( node::LITERAL );
	r->type = "literal";
	r->value = value;
	//	cout<<"mkliteral value: "<<value<<endl;
	r->datatype = datatype ? *datatype : XSD_STRING;
	if ( language ) r->lang = *language;
	return r;
}

pnode mkiri ( string iri ) {
	pnode r = make_shared <node> ( node::IRI );
	r->type = "IRI";
	r->value = iri;
	//	cout<<"mkiri value: "<<iri<<endl;
	return r;
}

pnode mkbnode ( string attribute ) {
	pnode r = make_shared <node> ( node::BNODE );
	r->type = "blank node";
	//	cout<<"mkbnode value: "<<attribute<<endl;
	r->value = attribute;
	return r;
}
/*
    somap node::toObject ( bool useNativeTypes ) {
    if ( type==IRI || type==BNODE ) { somap r; r[str_id]=mk_str_obj(at("value"));return r; }
    somap rval;
    rval[ str_value ] = mk_str_obj(at ( "value" ));
    auto it = find ( "language" );
    if ( it != end() ) rval[ str_lang ] = mk_str_obj(it->second);
    else {
    string type = at ( "datatype" ), value = at ( "value" );
    if ( useNativeTypes ) {
    if ( XSD_STRING == type ) {
    } else if ( XSD_BOOLEAN == type  ) {
    if ( value == "true"  ) rval[ str_value] = make_shared<bool_obj> ( true );
    else if ( value == "false" ) rval[ str_value] = make_shared<bool_obj> ( false );
    else rval[ str_type] = mk_str_obj(type);
    } else if ( ( XSD_INTEGER == type && is_int ( value ) ) ( XSD_DOUBLE == type && is_double ( value ) ) ) {
    double d = std::stod ( value );
    if ( !::isnan ( d ) && !::isinf ( d ) ) {
    if ( XSD_INTEGER == type ) {
    int64_t i = d;
    if ( tostr ( i ) == value ) rval[ str_value] = make_shared<uint64_t_obj> ( i );
    } else if ( XSD_DOUBLE == type )
    rval[ str_value] = make_shared<double_obj> ( d );
    else throw "This should never happen as we checked the type was either integer or double";
    }
    } else rval[str_type] = mk_str_obj ( type );
    } else if ( XSD_STRING != type  ) rval[str_type] = mk_str_obj ( type );
    }
    return rval;
    }
*/

const pnode first = mkiri ( RDF_FIRST );
const pnode rest = mkiri ( RDF_REST );
const pnode nil = mkiri ( RDF_NIL );

rdf_db::rdf_db ( jsonld_api& api_ ) :
	qdb(), api ( api_ ) {
	( *this ) [str_default] = mk_qlist();
}

string rdf_db::tostring() {
	string s;
	stringstream o;
	o << "#Graphs: " << size() << endl;
	for ( auto x : *this ) {
		o << "#Triples: " << x.second->size() << endl;
		for ( pquad q : *x.second )
			o << q->tostring ( x.first == str_default ? "<>" : x.first ) << endl;
	}
	return o.str();
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
					trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
					for ( int i = 0; i < ( ( int ) list->size() ) - 1; ++i ) {
						pnode object = obj_to_rdf ( list->at ( i ) );
						triples.push_back ( make_shared <quad> ( firstBnode, first, object, graph_name ) );
						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
						pnode restBnode = mkbnode ( api.gen_bnode_id() );
						triples.push_back ( make_shared <quad> ( firstBnode, rest, restBnode, graph_name ) );
						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
						firstBnode = restBnode;
					}
					if ( last ) {
						triples.push_back ( make_shared <quad> ( firstBnode, first, last, graph_name ) );
						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
						triples.push_back ( make_shared <quad> ( firstBnode, rest, nil, graph_name ) );
						trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
					}
				} else if ( pnode object = obj_to_rdf ( item ) ) {
					triples.push_back ( make_shared <quad> ( subj, pred, object, graph_name ) );
					trace ( "triple added: " << ( *triples.rbegin() )->tostring() << endl );
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
