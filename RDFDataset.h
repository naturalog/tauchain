#include "JsonLdConsts.h"
#include "JsonLdApi.h"
#include <set>
/**
    Starting to migrate away from using plain java Maps as the internal RDF
    dataset store. Currently each item just wraps a Map based on the old format
    so everything doesn't break. Will phase this out once everything is using the
    new format.

    @author Tristan

*/
struct Pattern {
	template<typename T> bool matcher ( T ) {
		return false;
	}
} PATTERN_INTEGER, PATTERN_DOUBLE;

class Node : public LinkedHashMap<String, Object> {
	//	static const long serialVersionUID = 1460990331795672793L;

public:
	virtual boolean isLiteral() const = 0;
	virtual boolean isIRI() const = 0;
	virtual boolean isBlankNode() const = 0;
	String getValue() const {
		return get ( "value" ).str();
	}
	String getDatatype() const {
		return get ( "datatype" ).str();
	}
	String getLanguage() const {
		return get ( "language" ).str();
	}
	bool operator< ( const Node& o ) const {
		if ( isIRI() ) {
			if ( !o.isIRI() ) {
				// IRIs > everything
				return false;
			}
		} else if ( isBlankNode() && ( o.isIRI() || o.isLiteral() ) ) return o.isIRI(); // IRI > blank node, blank node > literal
		return getValue() < o.getValue();
	}

	/**
	    Converts an RDF triple object to a JSON-LD object.

	    @param o
	              the RDF triple object to convert.
	    @param useNativeTypes
	              true to output native types, false not to.

	    @return the JSON-LD object.
	    @
	*/
	Map<String, Object> toObject ( Boolean useNativeTypes )  {
		// If value is an an IRI or a blank node identifier, return a new
		// JSON object consisting
		// of a single member @id whose value is set to value.
		if ( isIRI() || isBlankNode() ) return newMap ( "@id", getValue() );
		// convert literal object to JSON-LD
		Map<String, Object> rval = newMap ( "@value", getValue() );
		// add language
		if ( getLanguage().size() ) rval.put ( "@language", getLanguage() );
		// add datatype
		else {
			String type = getDatatype();
			String value = getValue();
			if ( useNativeTypes ) {
				// use native datatypes for certain xsd types
				if ( XSD_STRING == type ) {
					// don't add xsd:string
				} else if ( XSD_BOOLEAN == type ) {
					if ( "true"s == value  )
						rval.put ( "@value", true );
					else if ( "false"s == value )
						rval.put ( "@value", false );
					else // Else do not replace the value, and add the
						// boolean type in
						rval.put ( "@type", type );
				} else if (
				    // http://www.w3.org/TR/xmlschema11-2/#integer
				    <<< <<< < HEAD
				    ( XSD_INTEGER == type  && PATTERN_INTEGER.matcher ( value ) /*.matches()*/ )
				    // http://www.w3.org/TR/xmlschema11-2/#nt-doubleRep
				    || ( XSD_DOUBLE == type && PATTERN_DOUBLE.matcher ( value ) /*.matches()*/ ) ) {
					//					try {
					Double d = std::stod ( value );
					if ( !isnan ( d ) && !isinf ( d ) ) {
						if ( XSD_INTEGER == type ) {
							const Integer i = d; // conversion from double to integer
							if ( std::to_string ( i ) == value ) rval.put ( "@value", i );
						} else if ( XSD_DOUBLE == type )
							rval.put ( "@value", d );
						else throw std::runtime_error ( "This should never happen as we checked the type was either integer or double" );
					}
					// do not add xsd:string type
					else rval.put ( "@type", type );
				} else if ( XSD_STRING != type ) rval.put ( "@type", type );
			}
		}
		return rval;
	}
};

class RDFDataset : public LinkedHashMap<String, Node*> {
	//	static const long serialVersionUID = 2796344994239879165L;

	//	    static const boost::regex PATTERN_INTEGER ( "^[\\-+]?[0-9]+$" );
	//	static const boost::regex PATTERN_DOUBLE  ( "^(\\+|-)?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([Ee](\\+|-)?[0-9]+)?$" );

	class Quad : public LinkedHashMap<String, Object> { // : public Comparable<Quad> {
		//		static const long serialVersionUID = -7021918051975883082L;

	public:
		typedef LinkedHashMap<String, Object> base_t;
		Quad ( const String subject, const String predicate, const String object, const String graph ) :
			Quad ( subject, predicate, startsWith ( object, "_:" ) ? new BlankNode ( object ) : new IRI ( object ), graph ) {
		}

		Quad ( const String subject, const String predicate, const String value, const String datatype, const String language, const String graph )
			: Quad ( subject, predicate, new Literal ( value, datatype, language ), graph ) {
		}

	private:
		Quad ( const String subject, const String predicate, const Node& object, const String graph ) :
			Quad ( subject.startsWith ( "_:" ) ? new BlankNode ( subject ) : new IRI ( subject ), new IRI (
			           predicate ), object, graph ) {
		}

	public:
		Quad ( const Node& subject, const Node& predicate, const Node& object, const String graph ) : base_t() {
			put ( "subject", subject );
			put ( "predicate", predicate );
			put ( "object", object );
			if ( graph != null && !"@default".equals ( graph ) ) {
				// TODO: i'm not yet sure if this should be added or if the
				// graph should only be represented by the keys in the dataset
				put ( "name", graph.startsWith ( "_:" ) ? new BlankNode ( graph ) : new IRI ( graph ) );
			}
		}

		Node& getSubject() {
			return ( Node ) get ( "subject" );
		}

		Node& getPredicate() {
			return ( Node ) get ( "predicate" );
		}

		Node& getObject() {
			return ( Node ) get ( "object" );
		}

		Node& getGraph() {
			return ( Node ) get ( "name" );
		}

		bool operator< ( const Quad& o ) {
			//virtual int compareTo ( Quad o ) {
			if ( o == null ) return 1;
			int rval = getGraph() < o.getGraph() ;
			if ( getGraph() != o.getGraph() ) return getGraph() < o.getGraph();
			if ( getSubject() != o.getSubject() ) return getSubject() < o.getSubject();
			if ( getPredicate() != o.getPredicate() ) return getPredicate() < o.getPredicate();
			return getObject() < o.getObject();
		}
	};

	class Literal : public Node {
		static const long serialVersionUID = 8124736271571220251L;

	public:
		Literal ( String value, String datatype, String language ) : Node() {
			put ( "type", "literal" );
			put ( "value", value );
			put ( "datatype", datatype != null ? datatype : XSD_STRING );
			if ( language != null ) put ( "language", language );
		}

		virtual boolean isLiteral() const {
			return true;
		}
		virtual boolean isIRI() const {
			return false;
		}
		virtual  boolean isBlankNode() const {
			return false;
		}
		bool operator< ( const Node& o ) const {
			//		virtual  int compareTo ( Node o ) const {
			if ( o == null )
				// valid nodes are > null nodes
				return false;
			if ( o.isIRI() )
				// literals < iri
				return true;
			if ( o.isBlankNode() )
				// blank node < iri
				return true;
			if ( getLanguage() == null && ( ( Literal ) o ).getLanguage() != null )
				return true;
			else if ( getLanguage() != null && ( ( Literal ) o ).getLanguage() == null )
				return false;

			if ( getDatatype() != null )
				return getDatatype().compareTo ( ( ( Literal ) o ).getDatatype() );
			else if ( ( ( Literal ) o ).getDatatype() != null )
				return true;
			//			return 0;
		}
	};

	class IRI : public Node {
		static const long serialVersionUID = 1540232072155490782L;

	public:
		IRI ( String iri ) : Node() {
			put ( "type", "IRI" );
			put ( "value", iri );
		}

		virtual  boolean isLiteral() const {
			return false;
		}
		virtual  boolean isIRI() const {
			return true;
		}
		virtual  boolean isBlankNode() const {
			return false;
		}
	};

	class BlankNode : public Node {
		static const long serialVersionUID = -2842402820440697318L;

	public:
		BlankNode ( String attribute ) : Node() {
			put ( "type", "blank node" );
			put ( "value", attribute );
		}

		virtual boolean isLiteral() const {
			return false;
		}
		virtual  boolean isIRI() const {
			return false;
		}
		virtual  boolean isBlankNode() const {
			return true;
		}
	};

private:
	static const Node& first;//= IRI ( RDF_FIRST );
	static const Node& rest;// = IRI ( RDF_REST );
	static const Node& nil;// = IRI ( RDF_NIL );

	const Map<String, String> context;

	// private: UniqueNamer namer;
	JsonLdApi api;

public:
	RDFDataset() {
		super();
		put ( "@default", new ArrayList<Quad>() );
		context = new LinkedHashMap<String, String>();
		// put("@context", context);
	}

	/*
	    public: RDFDataset(String blankNodePrefix) { this(new
	    UniqueNamer(blankNodePrefix)); }

	    public: RDFDataset(UniqueNamer namer) { this(); this.namer = namer; }
	*/
	RDFDataset ( JsonLdApi jsonLdApi ) {
		this();
		this.api = jsonLdApi;
	}

	void setNamespace ( String ns, String prefix ) {
		context.put ( ns, prefix );
	}
	String getNamespace ( String ns ) {
		return context.get ( ns );
	}
	void clearNamespaces() {
		context.clear();
	}
	Map<String, String> getNamespaces() {
		return context;
	}
	/**
	    Returns a valid context containing any namespaces set

	    @return The context map
	*/
	Map<String, Object> getContext() {
		const Map<String, Object> rval = newMap();
		rval.putAll ( context );
		// replace "" with "@vocab"
		if ( rval.containsKey ( "" ) )
			rval.put ( "@vocab", rval.remove ( "" ) );
		return rval;
	}

	/**
	    parses a context object and sets any namespaces found within it

	    @param contextLike
	              The context to parse
	    @
	               If the context can't be parsed
	*/
	void parseContext ( Object contextLike )  {
		Context context;
		if ( api != null )
			context = new Context ( api.opts ); else
			context = new Context();
		// Context will do our recursive parsing and initial IRI resolution
		context = context.parse ( contextLike );
		// And then leak to us the potential 'prefixes'
		const Map<String, String> prefixes = context.getPrefixes ( true );

		for ( const String key : prefixes.keySet() ) {
			const String val = prefixes.get ( key );
			if ( "@vocab".equals ( key ) ) {
				if ( val == null || isString ( val ) )
					setNamespace ( "", val );
				else {
				}
			} else if ( !isKeyword ( key ) ) {
				setNamespace ( key, val );
				// TODO: should we make sure val is a valid URI prefix (i.e. it
				// ends with /# or ?)
				// or is it ok that full URIs for terms are used?
			}
		}
	}

	/**
	    Adds a triple to the @default graph of this dataset

	    @param subject
	              the subject for the triple
	    @param predicate
	              the predicate for the triple
	    @param value
	              the value of the literal object for the triple
	    @param datatype
	              the datatype of the literal object for the triple (null values
	              will default to xsd:string)
	    @param language
	              the language of the literal object for the triple (or null)
	*/
	void addTriple ( const String subject, const String predicate, const String value,
	                 const String datatype, const String language ) {
		addQuad ( subject, predicate, value, datatype, language, "@default" );
	}

	/**
	    Adds a triple to the specified graph of this dataset

	    @param s
	              the subject for the triple
	    @param p
	              the predicate for the triple
	    @param value
	              the value of the literal object for the triple
	    @param datatype
	              the datatype of the literal object for the triple (null values
	              will default to xsd:string)
	    @param graph
	              the graph to add this triple to
	    @param language
	              the language of the literal object for the triple (or null)
	*/
	void addQuad ( const String s, const String p, const String value, const String datatype,
	               const String language, String graph ) {
		if ( graph == null )
			graph = "@default";
		if ( !containsKey ( graph ) )
			put ( graph, new ArrayList<Quad>() );
		( ( ArrayList<Quad> ) get ( graph ) ).add ( new Quad ( s, p, value, datatype, language, graph ) );
	}

	/**
	    Adds a triple to the default graph of this dataset

	    @param subject
	              the subject for the triple
	    @param predicate
	              the predicate for the triple
	    @param object
	              the object for the triple
	*/
	void addTriple ( const String subject, const String predicate, const String object ) {
		addQuad ( subject, predicate, object, "@default" );
	}

	/**
	    Adds a triple to the specified graph of this dataset

	    @param subject
	              the subject for the triple
	    @param predicate
	              the predicate for the triple
	    @param object
	              the object for the triple
	    @param graph
	              the graph to add this triple to
	*/
	void addQuad ( const String subject, const String predicate, const String object,
	               String graph ) {
		if ( graph == null )
			graph = "@default";
		if ( !containsKey ( graph ) )
			put ( graph, new ArrayList<Quad>() );
		( ( ArrayList<Quad> ) get ( graph ) ).add ( new Quad ( subject, predicate, object, graph ) );
	}

	/**
	    Creates an array of RDF triples for the given graph.

	    @param graphName
	              The graph URI
	    @param graph
	              the graph to create RDF triples for.
	*/
	void graphToRDF ( String graphName, Map<String, Object> graph ) {
		// 4.2)
		const List<Quad> triples = new ArrayList<Quad>();
		// 4.3)
		const List<String> subjects = new ArrayList<String> ( graph.keySet() );
		// Collections.sort(subjects);
		for ( const String id : subjects ) {
			if ( JsonLdUtils.isRelativeIri ( id ) )
				continue;
			const Map<String, Object> node = ( Map<String, Object> ) graph.get ( id );
			const List<String> properties = new ArrayList<String> ( node.keySet() );
			Collections.sort ( properties );
			for ( String property : properties ) {
				const List<Object> values;
				// 4.3.2.1)
				if ( "@type".equals ( property ) ) {
					values = ( List<Object> ) node.get ( "@type" );
					property = RDF_TYPE;
				}
				// 4.3.2.2)
				else if ( isKeyword ( property ) )
					continue;
				// 4.3.2.3)
				else if ( property.startsWith ( "_:" ) && !api.opts.getProduceGeneralizedRdf() )
					continue;
				// 4.3.2.4)
				else if ( JsonLdUtils.isRelativeIri ( property ) )
					continue; else
					values = ( List<Object> ) node.get ( property );

				Node subject;
				if ( id.indexOf ( "_:" ) == 0 ) {
					// NOTE: don't rename, just set it as a blank node
					subject = new BlankNode ( id );
				} else
					subject = new IRI ( id );

				// RDF predicates
				Node predicate;
				if ( property.startsWith ( "_:" ) )
					predicate = new BlankNode ( property ); else
					predicate = new IRI ( property );

				for ( const Object item : values ) {
					// convert @list to triples
					if ( isList ( item ) ) {
						const List<Object> list = ( List<Object> ) ( ( Map<String, Object> ) item )
						                          .get ( "@list" );
						Node last = null;
						Node firstBNode = nil;
						if ( !list.isEmpty() ) {
							last = objectToRDF ( list.get ( list.size() - 1 ) );
							firstBNode = new BlankNode ( api.generateBlankNodeIdentifier() );
						}
						triples.add ( new Quad ( subject, predicate, firstBNode, graphName ) );
						for ( int i = 0; i < list.size() - 1; i++ ) {
							const Node object = objectToRDF ( list.get ( i ) );
							triples.add ( new Quad ( firstBNode, first, object, graphName ) );
							const Node restBNode = new BlankNode ( api.generateBlankNodeIdentifier() );
							triples.add ( new Quad ( firstBNode, rest, restBNode, graphName ) );
							firstBNode = restBNode;
						}
						if ( last != null ) {
							triples.add ( new Quad ( firstBNode, first, last, graphName ) );
							triples.add ( new Quad ( firstBNode, rest, nil, graphName ) );
						}
					}
					// convert value or node object to triple
					else {
						const Node object = objectToRDF ( item );
						if ( object != null )
							triples.add ( new Quad ( subject, predicate, object, graphName ) );
					}
				}
			}
		}
		put ( graphName, triples );
	}

	/**
	    Converts a JSON-LD value object to an RDF literal or a JSON-LD string or
	    node object to an RDF resource.

	    @param item
	              the JSON-LD value or node object.
	    @return the RDF literal or RDF resource.
	*/
private:
	Node* objectToRDF ( Object item ) {
		// convert value object to RDF
		if ( isValue ( item ) ) {
			const Object value = ( ( Map<String, Object> ) item ).get ( "@value" );
			const Object datatype = ( ( Map<String, Object> ) item ).get ( "@type" );

			// convert to XSD datatypes as appropriate
			if ( value.isBoolean || value.isNumber ) {
				// convert to XSD datatype
				if ( value.isBoolean ) {
					return new Literal ( value.toString(), datatype == null ? XSD_BOOLEAN
					                     : ( String ) datatype, null );
				} else if ( value.isDouble || value.isFloat
				            || XSD_DOUBLE.equals ( datatype ) ) {
					// canonical double representation
					const DecimalFormat df = new DecimalFormat ( "0.0###############E0" );
					df.setDecimalFormatSymbols ( DecimalFormatSymbols.getInstance ( Locale.US ) );
					return new Literal ( df.format ( value ), datatype == null ? XSD_DOUBLE
					                     : ( String ) datatype, null );
				} else {
					const DecimalFormat df = new DecimalFormat ( "0" );
					return new Literal ( df.format ( value ), datatype == null ? XSD_INTEGER
					                     : ( String ) datatype, null );
				}
			} else if ( ( ( Map<String, Object> ) item ).containsKey ( "@language" ) ) {
				return new Literal ( ( String ) value, datatype == null ? RDF_LANGSTRING
				                     : ( String ) datatype, ( String ) ( ( Map<String, Object> ) item ).get ( "@language" ) );
			} else {
				return new Literal ( ( String ) value, datatype == null ? XSD_STRING
				                     : ( String ) datatype, null );
			}
		}
		// convert string/node object to RDF
		else {
			const String id;
			if ( isObject ( item ) ) {
				id = ( String ) ( ( Map<String, Object> ) item ).get ( "@id" );
				if ( JsonLdUtils.isRelativeIri ( id ) )
					return null;
			} else
				id = ( String ) item;
			if ( id.indexOf ( "_:" ) == 0 ) {
				// NOTE: once again no need to rename existing blank nodes
				return new BlankNode ( id );
			} else
				return new IRI ( id );
		}
	}

public:
	std::set<String> graphNames() {
		// TODO Auto-generated method stub
		return keySet();
	}

	List<Quad> getQuads ( String graphName ) {
		return ( List<Quad> ) get ( graphName );
	}
};
