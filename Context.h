#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "JsonLdOptions.h"

template<typename Object>
class Context : public LinkedHashMap<String, Object> {
	JsonLdOptions options;
	Map<String, Object> termDefinitions;
	typedef LinkedHashMap<String, Object> base_t;
public:
	virtual bool isContext() const {
		return true;
	}
	using base_t::base_t;
	Map<String, Object> inverse;// = null;

	Context ( Map<String, Object> map = base_t(), JsonLdOptions opts = JsonLdOptions() ) : base_t ( map ) {
		init ( opts );
	}

	Context ( Object context, JsonLdOptions opts ) : Context ( context.isMap() ? context.obj() : base_t() /*null*/ ) {
		init ( opts );
	}
	Context ( JsonLdOptions opts ) {
		init ( opts );
	}

private:
	void init ( JsonLdOptions options_ ) {
		options = options_;
		if ( options.getBase().size() ) put ( "@base", options.getBase() );
		termDefinitions = base_t();
	}

public:
	/**
	    Value Compaction Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#value-compaction

	    @param activeProperty
	              The Active Property
	    @param value
	              The value to compact
	    @return The compacted value
	*/
	Object compactValue ( String activeProperty, Map<String, Object> value );
	/**
	    Context Processing Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#context-processing-algorithms

	    @param localContext
	              The Local Context object.
	    @param remoteContexts
	              The list of Strings denoting the remote Context URLs.
	    @return The parsed and merged Context.
	    @
	               If there is an error parsing the contexts.
	*/
	Context parse ( Object localContext, List<String> remoteContexts ) ;

	Context parse ( Object localContext )  {
		return parse ( localContext, new ArrayList<String>() );
	}

	/**
	    Create Term Definition Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#create-term-definition

	    @param result
	    @param context
	    @param key
	    @param defined
	    @
	*/
private:
	void createTermDefinition ( Map<String, Object> context, String term, Map<String, Boolean> defined );

	/**
	    IRI Expansion Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#iri-expansion

	    @param value
	    @param relative
	    @param vocab
	    @param context
	    @param defined
	    @return
	    @
	*/
	String expandIri ( String value, boolean relative, boolean vocab, Map<String, Object> context, Map<String, Boolean> defined );

	/**
	    IRI Compaction Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#iri-compaction

	    Compacts an IRI or keyword into a term or prefix if it can be. If the IRI
	    has an associated value it may be passed.

	    @param iri
	              the IRI to compact.
	    @param value
	              the value to check or null.
	    @param relativeTo
	              options for how to compact IRIs: vocab: true to split after
	    @vocab, false not to.
	    @param reverse
	              true if a reverse property is being compacted, false if not.

	    @return the compacted term, prefix, keyword alias, or the original IRI.
	*/
	String compactIri ( String iri, Object value, boolean relativeToVocab, boolean reverse );

	/**
	    Return a map of potential RDF prefixes based on the JSON-LD Term
	    Definitions in this context.
	    <p>
	    No guarantees of the prefixes are given, beyond that it will not contain
	    ":".

	    @param onlyCommonPrefixes
	              If <code>true</code>, the result will not include
	              "not so useful" prefixes, such as "term1":
	              "http://example.com/term1", e.g. all IRIs will end with "/" or
	              "#". If <code>false</code>, all potential prefixes are
	              returned.

	    @return A map from prefix string to IRI string
	*/
	Map<String, String> getPrefixes ( boolean onlyCommonPrefixes );

	String compactIri ( String iri, boolean relativeToVocab ) {
		return compactIri ( iri, null, relativeToVocab, false );
	}

	String compactIri ( String iri ) {
		return compactIri ( iri, null, false, false );
	}

	virtual Context clone() {
		const Context rval = ( Context ) super.clone();
		// TODO: is this shallow copy enough? probably not, but it passes all
		// the tests!
		rval.termDefinitions = LinkedHashMap<String, Object> ( termDefinitions );
		return rval;
	}

	/**
	    Inverse Context Creation

	    http://json-ld.org/spec/latest/json-ld-api/#inverse-context-creation

	    Generates an inverse context for use in the compaction algorithm, if not
	    already generated for the given active context.

	    @return the inverse context.
	*/
	Map<String, Object> getInverse();

	/**
	    Term Selection

	    http://json-ld.org/spec/latest/json-ld-api/#term-selection

	    This algorithm, invoked via the IRI Compaction algorithm, makes use of an
	    active context's inverse context to find the term that is best used to
	    compact an IRI. Other information about a value associated with the IRI
	    is given, including which container mappings and which type mapping or
	    language mapping would be best used to express the value.

	    @return the selected term.
	*/
	String selectTerm ( String iri, List<String> containers, String typeLanguage, List<String> preferredValues );

public:
	/**
	    Retrieve container mapping.

	    @param property
	              The Property to get a container mapping for.
	    @return The container mapping
	*/
	String getContainer ( String property ) {
		if ( "@graph".equals ( property ) )
			return "@set";
		if ( JsonLdUtils.isKeyword ( property ) )
			return property;
		const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
		if ( td == null )
			return null;
		return ( String ) td.get ( "@container" );
	}

	Boolean isReverseProperty ( String property ) {
		const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
		if ( td == null )
			return false;
		const Object reverse = td.get ( "@reverse" );
		return reverse != null && ( Boolean ) reverse;
	}

private:
	String getTypeMapping ( String property ) {
		const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
		if ( td == null )
			return null;
		return ( String ) td.get ( "@type" );
	}

	String getLanguageMapping ( String property ) {
		const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
		if ( td == null )
			return null;
		return ( String ) td.get ( "@language" );
	}

	Map<String, Object> getTermDefinition ( String key ) {
		return ( ( Map<String, Object> ) termDefinitions.get ( key ) );
	}

public:
	Object expandValue ( String activeProperty, Object value ) ;

	Object getContextValue ( String activeProperty, String string )  {
		throw new JsonLdError ( Error.NOT_IMPLEMENTED,
		                        "getContextValue is only used by old code so far and thus isn't implemented" );
	}

	Map<String, Object> serialize();
};
#endif
