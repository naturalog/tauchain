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

/*
IRI: 	 '<' value '>'

BNODE:	 '*' value '*'


LITERAL:
	https://www.w3.org/TR/rdf11-concepts/
	Section 3.3 Literals
 	//A literal in an RDF graph consists of two or three elements:
	//A literal is a language-tagged string if the third element is present.

	 '\"' value '\"^^' datatype
	 '\"' value '\"^^' datatype '@' lang

	//I think that means these two shouldn't be allowed.	
	 '\"' value '\"'
	 '\"' value '\"@' lang
		
	
*/
string node::tostring() const {
	std::stringstream ss;
	bool isiri = _type == IRI && !((*value).size() && ((*value)[0] == '?')) ;

	if ( isiri ) ss << '<';
	else if ( _type == BNODE ) ss << '*';
	else if ( _type == LITERAL ) ss << '\"';
	//else?

	//value is guaranteed to be filled?
	ss << *value;
	
	if ( isiri ) ss << '>';
	else if ( _type == BNODE ) ss << '*';
	else if ( _type == LITERAL ) {
		ss << '\"';
		if ( datatype && datatype->size() ) ss << "^^" << *datatype;
		if ( lang && lang->size() ) ss << '@' << *lang;
	}
	//is _type guaranteed to be filled?
	//else?


	return ss.str();
}


pnode set_dict(node r){	
	//If the dictionary already contains a node with this name,
	//return that one.
	//#dict.nodes : std::map<string,pnode>;

	string s = r.tostring();

	auto it = dict.nodes.find(s);
	if (it != dict.nodes.end()) {
		//TRACE(dout << "xxx" << s << "xxx " << it->second->_type << " " << r._type << endl);
		assert(it->second->_type == r._type);
		return it->second;
	}	
	//Otherwise put it into the dictionary and return
	//a shared pointer to it. Why is putting it into
	//dict.nodes not part of dict.set?
	pnode pr = make_shared<node>(r);
	dict.set(pr);
	return dict.nodes[s] = pr;
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



//
pnode mkiri ( pstring iri ) {
	setproc("mkiri");

	if (!iri) throw std::runtime_error("mkiri: null iri given");

	auto it = dict.nodes.find(*iri);
	if(it != dict.nodes.end())
		assert(it->second->_type == node::IRI);

	/// /	TRACE(dout << *iri << endl);
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

	auto it = dict.nodes.find(*attribute);
	if(it != dict.nodes.end())
		assert(it->second->_type == node::BNODE);

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
	//for ( auto x : q.second )we dont care about this much anymore
	//	o << x.first << ": " << std::endl << x.second << std::endl;
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




quad::quad ( pnode s, pnode p, pnode o, string c, bool auto_added_ ) :
	subj(s), pred(p), object(o), graph(startsWith ( c, "_:" ) ? mkbnode ( pstr(c) ) : mkiri ( pstr(c) ) ), auto_added (auto_added_)
{
		setproc("quad::ctor");
		TRACE(dout<<auto_added_<<auto_added<<":" << tostring()<<endl);
}




quad::quad ( pnode s, pnode p, pnode o, pnode c ) : quad(s, p, o, *c->value){}





string quad::tostring ( ) const {
	std::stringstream ss;
	//not really sure why this has to be a lambda
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


	ss << f ( subj ) << ' ' << f ( pred ) << ' ' << f ( object );
	if ( *graph->value != str_default ) ss << ' ' << f ( graph );
	ss << " .";
	return ss.str();
}



typedef map<nodeid, nodeid> BnodesDict;

bool nodes_same(BnodesDict &bnodes, pnode x, pnode y) {
	setproc("nodes_same");
	TRACE(dout << x->_type << ":" << x->tostring() << ", " <<
					  y->_type << ":" << y->tostring()  << endl);
	if (x->_type != y->_type)
		return false;
	if(x->_type == node::BNODE)
	{
		nodeid n1 = dict[x];
		nodeid n2 = dict[y];

		if (bnodes.find(n1) != bnodes.end())
		{
			return (bnodes[n1] == n2);
		}
		else
		{
			bnodes[n1] = n2;
			return true;
		}
	}
	//we'd end up here
	return *x == *y;
}

//let's make this into an '==' operator, or make an '==' operator that calls it
bool qdbs_equal(qdb &a, qdb &b) {
	FUN;
	TRACE(
	dout << "a.first.size  a.second.size  b.first.size  b.second.size" << endl;
	dout << a.first.size() << " " << a.second.size() << " " << b.first.size() << " " << b.second.size() << endl;
	)
	dout << "A:" << endl;
	dout << a;
	dout << "B:" << endl;
	dout << b;

	BnodesDict bnodes;

  //qlist
	auto a_default = *a.first["@default"];
	auto b_default = *b.first["@default"];
	auto i = a_default.begin();
	if (i == a_default.end())
			return false;

	pquad qa;
	for (pquad qb: b_default) {
		qa = *i;
		if (!(*qa->pred == *qb->pred))
			return false;
		if (!nodes_same(bnodes, qa->subj, qb->subj))
			return false;
		if (!nodes_same(bnodes, qa->object, qb->object))
			return false;
		i++;
	}
	return true;
/*
	for (pquad x: b_default) {
		//if (dict[x->pred] == rdffirst || dict[x->pred] == rdfrest)
		//	continue;

		//could move this in front of the loop
		if (i == a_default.end())
			return false;
		//would be better to call this na and nb or qa and
		pquad n1 = *i;
		pquad n2 = x;
		if (!(*n1->pred == *n2->pred))
			return false;
		if (!nodes_same(bnodes, n1->subj, a, n2->subj, b))
			return false;
		if (!nodes_same(bnodes, n1->object, a, n2->object, b))
			return false;
		i++;
	}
	return true;
	*/
}

qdb merge_qdbs(const std::vector<qdb> qdbs)
{
        qdb r;
		if (qdbs.size() == 0)
			return r;
		else if (qdbs.size() == 1)
			return qdbs[0];
		else
			dout << "warning, kb merging is half-assed";

        for (auto x:qdbs) {
			for (auto graph: x.first) {
				string name = graph.first;
				qlist contents = *graph.second;

				if (r.first.find(name) == r.first.end())
					r.first[name] = make_shared<qlist>(*new qlist());

				for (pquad c: contents) {
					r.first[name]->push_back(c);
				}
			}
			for (auto list: x.second) {
				string name = list.first;
				auto val = list.second;
				r.second[name] = val;
				dout << "warning, lists may get overwritten";
			}
		}

        return r;
}


