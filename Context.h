#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "defs.h"
#include "JsonLdOptions.h"
#include "JsonLdUrl.h"
#include "JsonLdError.h"
#include "JsonLdUtils.h"
#include "RemoteDocument.h"

template<typename Object>
class Context : public LinkedHashMap<String, Object> {
	JsonLdOptions<Object> options;
	Map<String, Object> termDefinitions;
	typedef LinkedHashMap<String, Object> base_t;
	//	void init ( JsonLdOptions<Object> options_ );
	//	String getTypeMapping ( String property );
	//	String getLanguageMapping ( String property );
	//	Map<String, Object> getTermDefinition ( String key ) ;
public:
	virtual bool isContext() const {
		return true;
	}
	using base_t::base_t;
	Map<String, Object> inverse;// = null;

	Context ( const Context& c );
	Context ( Map<String, Object> map = base_t(), JsonLdOptions<Object> opts = JsonLdOptions<Object>() ) : base_t ( map ) {
		init ( opts );
	}

	Context ( Object context, JsonLdOptions<Object> opts ) : Context ( context.isMap() ? context.obj() : base_t() /*null*/ ) {
		init ( opts );
	}
	Context ( JsonLdOptions<Object> opts ) {
		init ( opts );
	}

	Context parse ( Object localContext )  {
		return parse ( localContext, new ArrayList<String>() );
	}

	String compactIri ( String iri, boolean relativeToVocab ) {
		return compactIri ( iri, null, relativeToVocab, false );
	}

	/**
	    Removes the @preserve keywords as the last step of the framing algorithm.

	    @param ctx
	              the active context used to compact the input.
	    @param input
	              the framed, compacted output.
	    @param options
	              the compaction options used.

	    @return the resulting output.
	    @
	*/
	static Object removePreserve ( Context<Object>& ctx, Object& input, JsonLdOptions<Object>& opts )  {
		// recurse through arrays
		if ( isArray ( input ) ) {
			List<Object> output;
			for ( Object i : input.list() ) {
				Object result = removePreserve ( ctx, i, opts );
				// drop nulls from arrays
				if ( !result.is_null() ) output.push_back ( result );
			}
			input = output;
		} else if ( isObject ( input ) ) {
			// remove @preserve
			if ( input.obj( ).containsKey ( "@preserve" ) ) {
				if ( "@null"s == input.obj( ).get ( "@preserve" ).str() ) return null;
				return input.obj( ).get ( "@preserve" );
			}
			// skip @values
			if ( isValue ( input ) ) return input;
			// recurse through @lists
			if ( isList ( input ) ) {
				input.obj( ).put ( "@list", removePreserve ( ctx, input.obj( ).get ( "@list" ), opts ) );
				return input;
			}

			// recurse through properties
			//			for ( const String prop : ( input.obj()).keySet() ) {
			for ( auto x : input.obj() ) {
				String prop = x.first;
				Object result = removePreserve ( ctx, input.obj( ).get ( prop ), opts );
				const String container = ctx.getContainer ( prop );
				if ( opts.getCompactArrays() && isArray ( result )
				        && result.list( ).size() == 1 && !container.size() )
					result = result.list( ).at ( 0 );
				input.obj( ).put ( prop, result );
			}
		}
		return input;
	}

	virtual Context clone() {
		Context rval ( *this );
		// TODO: is this shallow copy enough? probably not, but it passes all
		// the tests!
		rval.termDefinitions = LinkedHashMap<String, Object> ( termDefinitions );
		return rval;
	}

public:
	Object compactValue ( String activeProperty, Map<String, Object> value ) {
		int numberMembers = value.size(); // 1
		if ( value.containsKey ( "@index" ) && "@index"s ==  getContainer ( activeProperty )  ) numberMembers--; // 2
		if ( numberMembers > 2 ) return value; // 3
		const String typeMapping = getTypeMapping ( activeProperty ); // 4
		const String languageMapping = getLanguageMapping ( activeProperty );
		if ( value.containsKey ( "@id" ) ) {
			if ( numberMembers == 1 && "@id"s == typeMapping ) return compactIri ( value.get ( "@id" ).str() ); // 4.1
			if ( numberMembers == 1 && "@vocab"s == typeMapping ) return compactIri ( value.get ( "@id" ).str(), true ); // 4.2
			return value; // 4.3
		}
		const Object valueValue = value.get ( "@value" );
		if ( value.containsKey ( "@type" ) && equals ( value.get ( "@type" ), typeMapping ) ) return valueValue; // 5
		if ( value.containsKey ( "@language" ) ) { // 6
			// TODO: SPEC: doesn't specify to check default language as well
			if ( equals ( value.get ( "@language" ), languageMapping ) || equals ( value.get ( "@language" ), base_t::get ( "@language" ) ) )
				return valueValue;
		}
		// 7)
		if ( numberMembers == 1
		        && ( ! ( valueValue.isString() ) || !base_t::containsKey ( "@language" ) || ( termDefinitions.containsKey ( activeProperty )
		                && getTermDefinition ( activeProperty ).containsKey ( "@language" ) && !languageMapping.size() /* == null */ ) ) )
			return valueValue;
		return value; // 8
	}

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
	Context<Object> parse ( Object localContext, List<String> remoteContexts )  {
		//		if ( !remoteContexts.size()/* == null*/ )
		//			remoteContexts = new ArrayList<String>();
		// 1. Initialize result to the result of cloning active context.
		Context<Object> result ( *this ); // TODO: clone?
		// 2)
		if ( !localContext.isList() ) {
			Object temp = localContext;
			localContext = Object ( ArrayList<Object>() );
			localContext.list().push_back ( temp );
		}
		// 3)
		for ( Object context : localContext.list() ) {
			// 3.1)
			if ( context == null ) {
				result = Context<Object> ( options );
				continue;
			} else if ( context.isContext() ) result = context.ctx( ).clone();
			// 3.2)
			else if ( context.isString() ) {
				String uri = result.get ( "@base" ).str();
				uri = JsonLdUrl::resolve ( uri, context.str() );
				// 3.2.2
				if ( std::find ( remoteContexts.begin(), remoteContexts.end(), uri ) != remoteContexts.end() )
					throw JsonLdError ( RECURSIVE_CONTEXT_INCLUSION, uri );
				remoteContexts.push_back ( uri );
				// 3.2.3: Dereference context
				RemoteDocument<Object> rd = options.getDocumentLoader().loadDocument ( uri );
				Object remoteContext = rd.document;
				if ( !remoteContext.isMap() || !remoteContext.obj( ).containsKey ( "@context" ) )
					// If the dereferenced document has no top-level JSON object with an @context member
					throw JsonLdError ( INVALID_REMOTE_CONTEXT, context );
				context = remoteContext.obj( ).get ( "@context" );
				result = result.parse ( context, remoteContexts ); // 3.2.4
				continue; // 3.2.5
			} else if ( ! context.isMap() ) throw JsonLdError ( INVALID_LOCAL_CONTEXT, context ); // 3.2
			// 3.4
			if ( !remoteContexts.size() && context.obj().containsKey ( "@base" ) ) {
				Object value = context.obj().get ( "@base" );
				if ( value == null ) result.remove ( "@base" );
				else if ( value.isString() ) {
					if ( JsonLdUtils<Object>::isAbsoluteIri ( value.str( ) ) )
						result.put ( "@base", value );
					else {
						String baseUri = result.get ( "@base" ).str();
						if ( !JsonLdUtils<Object>::isAbsoluteIri ( baseUri ) ) throw JsonLdError ( INVALID_BASE_IRI, baseUri );
						result.put ( "@base", JsonLdUrl::resolve ( baseUri, value.str( ) ) );
					}
				} else throw JsonLdError ( INVALID_BASE_IRI, "@base must be a string" );
			}
			// 3.5
			if ( context.obj().containsKey ( "@vocab" ) ) {
				const Object value = context.obj().get ( "@vocab" );
				if ( value == null )
					result.remove ( "@vocab" ); else if ( value.isString() ) {
					if ( JsonLdUtils<Object>::isAbsoluteIri ( ( String ) value ) )
						result.put ( "@vocab", value ); else
						throw JsonLdError ( INVALID_VOCAB_MAPPING, "@value must be an absolute IRI" );
				} else throw JsonLdError ( INVALID_VOCAB_MAPPING, "@vocab must be a string or null" );
			}
			// 3.6
			if ( context.obj().containsKey ( "@language" ) ) {
				const Object value = context.obj().get ( "@language" );
				if ( value == null ) result.remove ( "@language" );
				else if ( value.isString() ) result.put ( "@language", lower ( value.str( ) ) );
				else throw JsonLdError ( INVALID_DEFAULT_LANGUAGE, value );
			}
			// 3.7
			Map<String, Boolean> defined;// = new LinkedHashMap<String, Boolean>();
			for ( auto x : context.obj() ) { //const String key : context.obj().keySet() ) {
				String key = x.first;
				if ( is ( key, { "@base"s, "@vocab", "@language"s } ) ) continue;
				result.createTermDefinition ( context.obj(), key, defined );
			}
		}
		return result;
	}

	void createTermDefinition ( Map<String, Object>& context, String term, Map<String, Boolean>& defined )  {
		if ( defined.containsKey ( term ) ) {
			if ( defined.get ( term ) ) return;
			throw JsonLdError ( CYCLIC_IRI_MAPPING, term );
		}
		defined.put ( term, false );
		if ( JsonLdUtils<Object>::isKeyword ( term ) ) throw JsonLdError ( KEYWORD_REDEFINITION, term );
		termDefinitions.remove ( term );
		Object value = context.get ( term );
		if ( value == null || ( value.isMap() && value.obj( ).containsKey ( "@id" ) && value.obj( ) .get ( "@id" ) == null ) ) {
			termDefinitions.put ( term, null );
			defined.put ( term, true );
			return;
		}

		if ( value.isString() ) value = newMap ( "@id", value );
		if ( ! value.isMap() ) throw JsonLdError ( INVALID_TERM_DEFINITION, value );
		// casting the value so it doesn't have to be done below everytime
		const Map<String, Object> val = value.obj();
		// 9) create a new term definition
		const Map<String, Object> definition;// = newMap();
		// 10)
		if ( val.containsKey ( "@type" ) ) {
			if ( !  val.get ( "@type" ).isString()  ) throw JsonLdError ( INVALID_TYPE_MAPPING, val.get ( "@type" ).str() );
			String type = val.get ( "@type" ).str();
			try {
				type = expandIri ( ( String ) val.get ( "@type" ), false, true, context, defined );
			} catch ( const JsonLdError error ) {
				if ( error.getType() != INVALID_IRI_MAPPING ) throw error;
				throw JsonLdError ( INVALID_TYPE_MAPPING, type );
			}
			// TODO: fix check for absoluteIri (blank nodes shouldn't count, at
			// least not here!)
			if ( "@id"s == type  || "@vocab"s == type
			        || ( !startsWith ( type, "_:" ) && JsonLdUtils<Object>::isAbsoluteIri ( type ) ) )
				definition.put ( "@type", type );
			else throw JsonLdError ( INVALID_TYPE_MAPPING, type );
		}
		// 11)
		if ( val.containsKey ( "@reverse" ) ) {
			if ( val.containsKey ( "@id" ) ) throw JsonLdError ( INVALID_REVERSE_PROPERTY, val );
			if ( !  val.get ( "@reverse" ).isString()  )
				throw JsonLdError ( INVALID_IRI_MAPPING,
				                    "Expected String for @reverse value. got "
				                    + ( val.get ( "@reverse" ) == null ? "null" : val.get ( "@reverse" )
				                        .getClass() ) );

			const String reverse = expandIri ( val.get ( "@reverse" ).str(), false, true, context, defined );
			if ( !JsonLdUtils<Object>::isAbsoluteIri ( reverse ) ) throw JsonLdError ( INVALID_IRI_MAPPING, "Non-absolute @reverse IRI: " + reverse );
			definition.put ( "@id", reverse );
			if ( val.containsKey ( "@container" ) ) {
				const String container = val.get ( "@container" ).str();
				if ( !container.size() || is ( container, { "@set"s, "@index"s} ) ) definition.put ( "@container", container );
				else throw JsonLdError ( INVALID_REVERSE_PROPERTY, "reverse properties only support set- and index-containers" );
			}
			definition.put ( "@reverse", true );
			termDefinitions.put ( term, definition );
			defined.put ( term, true );
			return;
		}

		// 12)
		definition.put ( "@reverse", false );

		// 13)
		if ( val.get ( "@id" ) != null && term != val.get ( "@id" ) ) {
			if ( ! val.get ( "@id" ).isString() ) throw JsonLdError ( INVALID_IRI_MAPPING, "expected value of @id to be a string" );
			String res = expandIri ( val.get ( "@id" ).str(), false, true, context, defined );
			if ( JsonLdUtils<Object>::isKeyword ( res ) || JsonLdUtils<Object>::isAbsoluteIri ( res ) ) {
				if ( "@context"s == res ) throw JsonLdError ( INVALID_KEYWORD_ALIAS, "cannot alias @context" );
				definition.put ( "@id", res );
			} else
				throw JsonLdError ( INVALID_IRI_MAPPING, "resulting IRI mapping should be a keyword, absolute IRI or blank node" );
		}

		// 14)
		else if ( term.find ( ":" ) != String::npos ) {
			const int colIndex = term.find ( ":" );
			const String prefix = term.substr ( 0, colIndex );
			const String suffix = term.substr ( colIndex + 1 );
			if ( context.containsKey ( prefix ) ) createTermDefinition ( context, prefix, defined );
			definition.put ( "@id", termDefinitions.containsKey ( prefix ) ? termDefinitions.get ( prefix ).obj( ).get ( "@id" ) + suffix : term );
			// 15)
		} else if ( base_t::containsKey ( "@vocab" ) ) definition.put ( "@id", base_t::get ( "@vocab" ) + term );
		else throw JsonLdError ( INVALID_IRI_MAPPING, "relative term definition without vocab mapping" );

		// 16)
		if ( val.containsKey ( "@container" ) ) {
			String container = ( String ) val.get ( "@container" ).str();
			if ( !is ( container, { "@list"s, "@set"s, "@index"s, "@language"s} ) )
				throw JsonLdError ( INVALID_CONTAINER_MAPPING, "@container must be either @list, @set, @index, or @language" );
			definition.put ( "@container", container );
		}

		// 17)
		if ( val.containsKey ( "@language" ) && !val.containsKey ( "@type" ) ) {
			if ( val.get ( "@language" ) == null || val.get ( "@language" ).isString() ) {
				const String language = val.get ( "@language" ).str();
				definition.put ( "@language", lower ( language ) );
			} else throw JsonLdError ( INVALID_LANGUAGE_MAPPING, "@language must be a string or null" );
		}

		// 18)
		termDefinitions.put ( term, definition );
		defined.put ( term, true );
	}

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
	String expandIri ( String& value, boolean relative, boolean vocab, Map<String, Object>& context, Map<String, Boolean>& defined )  {
		if ( !value.size() || JsonLdUtils<Object>::isKeyword ( value ) ) return value; // 1
		if ( /* context != null &&*/ context.containsKey ( value ) && !defined.get ( value ) ) // 2
			createTermDefinition ( context, value, defined );
		// 3)
		if ( vocab && termDefinitions.containsKey ( value ) ) {
			const Map<String, Object> td = termDefinitions .get ( value ).obj();
			return td == null ? null : td.get ( "@id" ).str();
		}
		// 4)
		const int colIndex = value.find ( ":" );
		if ( colIndex >= 0 ) {
			// 4.1)
			const String prefix = value.substr ( 0, colIndex );
			const String suffix = value.substr ( colIndex + 1 );
			// 4.2)
			if ( "_"s == prefix || startsWith ( suffix, "//" ) ) return value;
			// 4.3)
			if ( context != null && context.containsKey ( prefix ) && ( !defined.containsKey ( prefix ) || !defined.get ( prefix ) ) )
				createTermDefinition ( context, prefix, defined );
			// 4.4)
			if ( termDefinitions.containsKey ( prefix ) ) return termDefinitions.get ( prefix ).obj( ) .get ( "@id" ).str() + suffix;
			return value; // 4.5
		}
		if ( vocab && base_t::containsKey ( "@vocab" ) ) return base_t::get ( "@vocab" ) + value; // 5
		// 6)
		if ( relative ) return JsonLdUrl::resolve ( base_t::get ( "@base" ).str(), value );
		if ( context != null && JsonLdUtils<Object>::isRelativeIri ( value ) ) throw JsonLdError ( INVALID_IRI_MAPPING, "not an absolute IRI: " + value );
		// 7)
		return value;
	}

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
	String compactIri ( String iri, Object value, boolean relativeToVocab, boolean reverse ) {
		//		if ( iri == null ) return null; // 1
		if ( !iri.size() ) return String();
		if ( relativeToVocab && getInverse().containsKey ( iri ) ) { // 2
			String defaultLanguage = base_t::get ( "@language" ).str(), typeLanguage = "@language", typeLanguageValue = "@null"; // 2.1,2.3
			List<String> containers; // 2.2
			if ( !defaultLanguage.size() ) defaultLanguage = "@none";
			if ( value.isMap() && value.obj( ).containsKey ( "@index" ) ) containers.push_back ( "@index" ); // 2.4
			if ( reverse ) { // 2.5
				typeLanguage = "@type";
				typeLanguageValue = "@reverse";
				containers.push_back ( "@set" );
			}
			// 2.6)
			else if ( value.isMap() && value.obj( ).containsKey ( "@list" ) ) {
				if ( !  value.obj( ).containsKey ( "@index" ) ) containers.push_back ( "@list" ); // 2.6.1
				List<Object> list = value.obj( ).get ( "@list" ).list(); // 2.6.2
				String commonType, commonLanguage;// 2.6.3
				if ( !list.size() ) commonLanguage = defaultLanguage;
				for ( const Object item : list ) {// 2.6.4
					String itemLanguage = "@none", itemType = "@none"; // 2.6.4.1
					if ( JsonLdUtils<Object>::isValue ( item ) ) { // 2.6.4.2
						if ( item.obj( ).containsKey ( "@language" ) ) itemLanguage = item.obj( ).get ( "@language" ).str(); // 2.6.4.2.1
						else if ( item.obj( ).containsKey ( "@type" ) ) itemType = item.obj( ).get ( "@type" ).str(); // 2.6.4.2.2
						else itemLanguage = "@null"; // 2.6.4.2.3
					} else itemType = "@id"; // 2.6.4.3
					if ( !commonLanguage.size() ) commonLanguage = itemLanguage; // 2.6.4.4
					else if ( commonLanguage != itemLanguage && JsonLdUtils<Object>::isValue ( item ) ) // 2.6.4.5
						commonLanguage = "@none";
					if ( !commonType.size() ) commonType = itemType; // 2.6.4.6
					else if ( commonType != itemType ) commonType = "@none";// 2.6.4.7)
					if ( "@none"s == commonLanguage && "@none"s == commonType ) break; // 2.6.4.8
				}
				commonLanguage = commonLanguage.size() ? commonLanguage : "@none"; // 2.6.5)
				commonType = commonType.size() ? commonType : "@none"; // 2.6.6
				if ( "@none"s != commonType ) { // 2.6.7
					typeLanguage = "@type";
					typeLanguageValue = commonType;
				} else typeLanguageValue = commonLanguage; // 2.6.8
			} else { // 2.7
				// 2.7.1)
				if ( value.isMap() && value.obj( ).containsKey ( "@value" ) ) {
					// 2.7.1.1)
					if ( value.obj( ).containsKey ( "@language" ) && ! value.obj( ).containsKey ( "@index" ) ) {
						containers.push_back ( "@language" );
						typeLanguageValue = value.obj( ).get ( "@language" ).str();
					} else if ( value.obj( ).containsKey ( "@type" ) ) { // 2.7.1.2
						typeLanguage = "@type";
						typeLanguageValue = value.obj( ).get ( "@type" ).str();
					}
				} else { // 2.7.2
					typeLanguage = "@type";
					typeLanguageValue = "@id";
				}
				containers.push_back ( "@set" ); // 2.7.3
			}

			containers.push_back ( "@none" ); // 2.8
			if ( !typeLanguageValue.size() ) typeLanguageValue = "@null"; // 2.9
			List<String> preferredValues; // 2.10
			if ( "@reverse"s == typeLanguageValue ) preferredValues.push_back ( "@reverse" ); // 2.11
			// 2.12)
			if ( is ( typeLanguageValue, { "@reverse"s, "@id"s} ) && ( value.isMap() ) && value.map().containsKey ( "@id" ) ) {
				// 2.12.1)
				const String result = compactIri ( value.map.get ( "@id" ).str(), null, true, true );
				if ( termDefinitions.containsKey ( result )
				        && termDefinitions.map().get ( result ) .map().containsKey ( "@id" )
				        && value.map() .get ( "@id" ).equals (
				            termDefinitions.map().get ( result ).map() .get ( "@id" ) ) ) {
					preferredValues.push_back ( "@vocab" );
					preferredValues.push_back ( "@id" );
				} else { // 2.12.2
					preferredValues.push_back ( "@id" );
					preferredValues.push_back ( "@vocab" );
				}
			}
			else preferredValues.push_back ( typeLanguageValue ); // 2.13
			preferredValues.push_back ( "@none" );

			String term = selectTerm ( iri, containers, typeLanguage, preferredValues ); // 2.14
			if ( term.size() ) return term; // 2.15
		}

		if ( relativeToVocab && base_t::containsKey ( "@vocab" ) ) { // 3
			// determine if vocab is a prefix of the iri
			const String vocab = base_t::get ( "@vocab" ).str();
			// 3.1)
			if ( iri.find ( vocab ) == 0 && iri != vocab ) {
				// use suffix as relative iri if it is not a term in the
				// active context
				const String suffix = iri.substr ( vocab.length() );
				if ( !termDefinitions.containsKey ( suffix ) ) return suffix;
			}
		}
		String compactIRI; // 4
		for ( auto x : termDefinitions ) { // 5
			String term = x.first;
			Map<String, Object> termDefinition = x.second.obj();//termDefinitions.map().get ( term ).map();
			if ( term.find ( ":" ) != String::npos ) continue; // 5.1
			// 5.2)
			if ( !termDefinition.size() || iri == termDefinition.get ( "@id" ).str() || !startsWith ( iri, termDefinition.get ( "@id" ).str() ) ) continue;
			const String candidate = term + ":" + iri.substr ( termDefinition.get ( "@id" ).str().length() ); // 5.3
			// 5.4)
			if ( ( !compactIRI.size() || JsonLdUtils<Object>::compareShortestLeast ( candidate, compactIRI ) < 0 )
			        && ( !termDefinitions.containsKey ( candidate ) || ( iri == termDefinitions.get ( candidate ).obj( ) .get ( "@id" ) && value == null ) ) )
				compactIRI = candidate;

		}
		if ( compactIRI.size() ) return compactIRI; // 6
		if ( !relativeToVocab ) return JsonLdUtils<Object>::removeBase ( base_t::get ( "@base" ), iri ); // 7
		return iri; // 8
	}

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
	Map<String, String> getPrefixes ( boolean onlyCommonPrefixes ) {
		Map<String, String> prefixes;
		for ( auto x : termDefinitions ) {
			String term = x.first;
			if ( term.find ( ":" ) != String::npos ) continue;
			Map<String, Object> termDefinition = termDefinitions .get ( term ).obj();
			if ( termDefinition == null ) continue;
			String id = termDefinition.get ( "@id" ).str();
			if ( !id.size() ) continue;
			if ( startsWith ( term, "@" ) || startsWith ( id, "@" ) ) continue;
			if ( !onlyCommonPrefixes || endsWith ( id, "/" ) || endsWith ( id, "#" ) ) prefixes.put ( term, id );
		}
		return prefixes;
	}

	String compactIri ( String iri ) {
		return compactIri ( iri, null, false, false );
	}

	/**
	    Inverse Context Creation

	    http://json-ld.org/spec/latest/json-ld-api/#inverse-context-creation

	    Generates an inverse context for use in the compaction algorithm, if not
	    already generated for the given active context.

	    @return the inverse context.
	*/
	Map<String, Object> getInverse() {
		// lazily create inverse
		if ( inverse != null ) return inverse;
		inverse = Map<String, Object>(); // 1
		String defaultLanguage = base_t::get ( "@language" ).str(); // 2
		if ( !defaultLanguage.size() ) defaultLanguage = "@none";

		// create term selections for each mapping in the context, ordererd by
		// shortest and then lexicographically least
		List<String> terms; for ( auto x : termDefinitions ) terms.push_back ( x.first );
		std::sort ( terms.begin(), terms.end(), [] ( String a, String b ) {
			return JsonLdUtils<Object>::compareShortestLeast ( a, b );
		} );

		for ( String term : terms ) {
			Map<String, Object> definition = termDefinitions.get ( term ).obj();
			if ( definition == null ) continue; // 3.1
			String container = ( String ) definition.get ( "@container" ).str(); // 3.2
			if ( !container.size() ) container = "@none";
			const String iri = ( String ) definition.get ( "@id" ).str(); // 3.3
			Map<String, Object> containerMap = inverse.get ( iri ).obj(); // 3.4,3.5
			if ( !containerMap.size() ) {
				containerMap = Map<String, Object>();//newMap();
				inverse.put ( iri, containerMap );
			}

			Map<String, Object> typeLanguageMap = containerMap.get ( container ).obj(); // 3.6,3.7
			if ( !typeLanguageMap.size() ) {
				typeLanguageMap = Map<String, Object>();
				typeLanguageMap.put ( "@language", Map<String, Object>() );
				typeLanguageMap.put ( "@type", Map<String, Object>() );
				containerMap.put ( container, typeLanguageMap );
			}

			if ( definition.get ( "@reverse" ) ) { // 3.8
				Map<String, Object> typeMap = typeLanguageMap .get ( "@type" ).obj();
				if ( !typeMap.containsKey ( "@reverse" ) ) typeMap.put ( "@reverse", term );
			} else if ( definition.containsKey ( "@type" ) ) { // 3.9
				Map<String, Object> typeMap = typeLanguageMap .get ( "@type" ).obj();
				if ( !typeMap.containsKey ( definition.get ( "@type" ) ) )
					typeMap.put ( definition.get ( "@type" ).str(), term );
			} else if ( definition.containsKey ( "@language" ) ) { // 3.10
				Map<String, Object> languageMap = typeLanguageMap .get ( "@language" ).obj();
				String language = definition.get ( "@language" ).str();
				if ( !language.size() ) language = "@null";
				if ( !languageMap.containsKey ( language ) ) languageMap.put ( language, term );
				// 3.11)
			} else {
				Map<String, Object> languageMap = typeLanguageMap .get ( "@language" ).obj(); // 3.11.1
				if ( !languageMap.containsKey ( "@language" ) ) languageMap.put ( "@language", term ); // 3.11.2
				if ( !languageMap.containsKey ( "@none" ) ) languageMap.put ( "@none", term ); // 3.11.3
				Map<String, Object> typeMap = typeLanguageMap .get ( "@type" ).obj(); // 3.11.4
				if ( !typeMap.containsKey ( "@none" ) ) typeMap.put ( "@none", term ); // 3.11.5
			}
		}
		// 4)
		return inverse;
	}

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
	String selectTerm ( String iri, List<String> containers, String typeLanguage, List<String> preferredValues ) {
		const Map<String, Object> inv = getInverse();
		const Map<String, Object> containerMap = ( Map<String, Object> ) inv.get ( iri ); // 1
		for ( const String container : containers ) { // 2
			if ( !containerMap.containsKey ( container ) ) continue; // 2.1
			const Map<String, Object> typeLanguageMap = containerMap .get ( container ).obj(); // 2.2
			const Map<String, Object> valueMap = typeLanguageMap .get ( typeLanguage ).obj(); // 2.3
			for ( const String item : preferredValues ) { // 2.4
				if ( !valueMap.containsKey ( item ) ) continue; // 2.4.1
				return ( String ) valueMap.get ( item ); // 2.4.2
			}
		}
		return String(); // 3
	}

	/**
	    Retrieve container mapping.

	    @param property
	              The Property to get a container mapping for.
	    @return The container mapping
	*/
	Object expandValue ( String activeProperty, Object value )  {
		const Map<String, Object> rval;
		const Map<String, Object> td = getTermDefinition ( activeProperty );
		// 1)
		if ( td != null && "@id"s == td.get ( "@type" ) )  {
			// TODO: i'm pretty sure value should be a string if the @type is
			// @id
			rval.put ( "@id", expandIri ( value.toString(), true, false, null, null ) );
			return rval;
		}
		// 2)
		if ( td != null && "@vocab"s == td.get ( "@type" ) ) {
			// TODO: same as above
			rval.put ( "@id", expandIri ( value.toString(), true, true, null, null ) );
			return rval;
		}
		// 3)
		rval.put ( "@value", value );
		// 4)
		if ( td != null && td.containsKey ( "@type" ) ) rval.put ( "@type", td.get ( "@type" ) );
		// 5)
		else if ( value.isString() ) {
			// 5.1)
			if ( td != null && td.containsKey ( "@language" ) ) {
				const String lang = td.get ( "@language" ).str();
				if ( lang.size() ) rval.put ( "@language", lang );
			}
			// 5.2)
			else if ( base_t::get ( "@language" ) != null ) rval.put ( "@language", base_t::get ( "@language" ) );
		}
		return rval;
	}
	/*
		Map<String, Object> serialize() {
			Map<String, Object> ctx;// = newMap();
			if ( base_t::get ( "@base" ) != null && base_t::get ( "@base" ) != options.getBase() ) ctx.put ( "@base", get ( "@base" ) );
			if ( base_t::get ( "@language" ) != null ) ctx.put ( "@language", get ( "@language" ) );
			if ( base_t::get ( "@vocab" ) != null ) ctx.put ( "@vocab", get ( "@vocab" ) );
	    //		for ( const String term : termDefinitions.keySet() ) {
			for (auto x : termDefinitions) {
				String term = x.first;
				const Map<String, Object> definition = x.second.obj*(;//termDefinitions.get ( term ).obj();
				if ( definition.get ( "@language" ) == null
				        && definition.get ( "@container" ) == null
				        && definition.get ( "@type" ) == null
				        && ( definition.get ( "@reverse" ) == null || !definition .get ( "@reverse" ).boolean()))
					const String cid = compactIri ( ( String ) definition.get ( "@id" ) );
					ctx.put ( term, term.equals ( cid ) ? definition.get ( "@id" ) : cid );
				} else {
					Map<String, Object> defn;// = newMap();
					String cid = compactIri ( definition.get ( "@id" ) ).str();
					Boolean reverseProperty = definition.get ( "@reverse" ).boolean();
					if (  term != cid  && !reverseProperty ) ) defn.put ( reverseProperty ? "@reverse" : "@id", cid );
					if ( definition.has_str("@type") ) {
						String typeMapping = definition.get ( "@type" ).str();
						defn.put ( "@type", JsonLdUtils<Object>::isKeyword ( typeMapping ) ? typeMapping : compactIri ( typeMapping, true ) );
					}
				}
				if ( definition.get ( "@container" ) != null ) defn.put ( "@container", definition.get ( "@container" ) );
				const Object lang = definition.get ( "@language" );
				if ( definition.get ( "@language" ) != null ) defn.put ( "@language", !lang ? null : lang );
				ctx.put ( term, defn );
			}

			Map<String, Object> rval;
			if ( !ctx.size() ) rval.put ( "@context", ctx );
			return rval;
		}
	*/
	String getContainer ( String property ) {
		if ( "@graph"s == property ) return "@set";
		if ( JsonLdUtils<Object>::isKeyword ( property ) ) return property;
		const Map<String, Object> td = termDefinitions.get ( property ).obj();
		if ( td == null ) return null;
		return td.get ( "@container" ).str();
	}

	Boolean isReverseProperty ( String property ) {
		Map<String, Object> td = termDefinitions.get ( property ).obj();
		if ( td == null ) return false;
		Object reverse = td.get ( "@reverse" );
		return reverse != null && reverse.boolean();
	}
	Object getContextValue ( String activeProperty, String string )  {
		throw JsonLdError ( NOT_IMPLEMENTED,
		                    "getContextValue is only used by old code so far and thus isn't implemented" );
	}
private:
	String getTypeMapping ( String property ) {
		Object o;
		if ( !termDefinitions.get ( property, o ) ) return String();
		if ( !o.is_str() ) return String();
		return o.obj().get ( "@type" ).str();
	}
	String getLanguageMapping ( String property ) {
		Object o;
		if ( !termDefinitions.get ( property, o ) ) return String();
		if ( !o.is_str() ) return String();
		return o.obj().get ( "@language" ).str();
	}
	Map<String, Object> getTermDefinition(String key) { return termDefinitions.get(key).obj(); }

};
#endif
