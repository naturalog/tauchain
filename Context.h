#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "defs.h"
#include "JsonLdOptions.h"

template<typename Object>
class Context : public LinkedHashMap<String, Object> {
	JsonLdOptions<Object> options;
	Map<String, Object> termDefinitions;
	typedef LinkedHashMap<String, Object> base_t;
	void init ( JsonLdOptions<Object> options_ );
	String getTypeMapping ( String property );
	String getLanguageMapping ( String property );
	Map<String, Object> getTermDefinition ( String key ) ;
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

	virtual Context clone() {
		Context rval ( *this );
		// TODO: is this shallow copy enough? probably not, but it passes all
		// the tests!
		rval.termDefinitions = LinkedHashMap<String, Object> ( termDefinitions );
		return rval;
	}

public:

	Object compactValue ( String activeProperty, Map<String, Object> value ) {
		// 1)
		int numberMembers = value.size();
		// 2)
		if ( value.containsKey ( "@index" ) && "@index"s ==  getContainer ( activeProperty )  )
			numberMembers--;
		// 3)
		if ( numberMembers > 2 )
			return value;
		// 4)
		const String typeMapping = getTypeMapping ( activeProperty );
		const String languageMapping = getLanguageMapping ( activeProperty );
		if ( value.containsKey ( "@id" ) ) {
			// 4.1)
			if ( numberMembers == 1 && "@id"s == ( typeMapping ) )
				return compactIri ( value.get ( "@id" ).str() );
			// 4.2)
			if ( numberMembers == 1 && "@vocab"s == ( typeMapping ) )
				return compactIri ( value.get ( "@id" ).str(), true );
			// 4.3)
			return value;
		}
		const Object valueValue = value.get ( "@value" );
		// 5)
		if ( value.containsKey ( "@type" ) && equals ( value.get ( "@type" ), typeMapping ) )
			return valueValue;
		// 6)
		if ( value.containsKey ( "@language" ) ) {
			// TODO: SPEC: doesn't specify to check default language as well
			if ( equals ( value.get ( "@language" ), languageMapping )
			        || equals ( value.get ( "@language" ), get ( "@language" ) ) )
				return valueValue;
		}
		// 7)
		if ( numberMembers == 1
		        && ( ! ( valueValue.isString() ) || !containsKey ( "@language" ) || ( termDefinitions
		                .containsKey ( activeProperty )
		                && getTermDefinition ( activeProperty ).containsKey ( "@language" ) && !languageMapping.size() /* == null */ ) ) )
			return valueValue;
		// 8)
		return value;
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
	Context parse ( Object localContext, List<String> remoteContexts )  {
		//		if ( !remoteContexts.size()/* == null*/ )
		//			remoteContexts = new ArrayList<String>();
		// 1. Initialize result to the result of cloning active context.
		Context result = clone(); // TODO: clone?
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
				result = Context ( options );
				continue;
			} else if ( context.isContext() )
				result = ( ( Context ) context ).clone();
			// 3.2)
			else if ( context.isString() ) {
				String uri = result.get ( "@base" ).str();
				uri = JsonLdUrl::resolve ( uri, context.str() );
				// 3.2.2
				if ( remoteContexts.contains ( uri ) )
					throw new JsonLdError ( Error.RECURSIVE_CONTEXT_INCLUSION, uri );
				remoteContexts.push_back ( uri );

				// 3.2.3: Dereference context
				const RemoteDocument rd = options.getDocumentLoader().loadDocument ( uri );
				const Object remoteContext = rd.document;
				if ( ! ( remoteContext.isMap() )
				        || ! ( ( Map<String, Object> ) remoteContext ).containsKey ( "@context" ) ) {
					// If the dereferenced document has no top-level JSON object
					// with an @context member
					throw new JsonLdError ( Error.INVALID_REMOTE_CONTEXT, context );
				}
				context = ( ( Map<String, Object> ) remoteContext ).get ( "@context" );

				// 3.2.4
				result = result.parse ( context, remoteContexts );
				// 3.2.5
				continue;
			} else if ( ! context.isMap() ) {
				// 3.3
				throw new JsonLdError ( Error.INVALID_LOCAL_CONTEXT, context );
			}

			// 3.4
			if ( remoteContexts.isEmpty() && ( ( Map<String, Object> ) context ).containsKey ( "@base" ) ) {
				const Object value = ( ( Map<String, Object> ) context ).get ( "@base" );
				if ( value == null )
					result.remove ( "@base" ); else if ( value.isString() ) {
					if ( JsonLdUtils.isAbsoluteIri ( ( String ) value ) )
						result.put ( "@base", value ); else {
						const String baseUri = ( String ) result.get ( "@base" );
						if ( !JsonLdUtils.isAbsoluteIri ( baseUri ) )
							throw new JsonLdError ( Error.INVALID_BASE_IRI, baseUri );
						result.put ( "@base", JsonLdUrl.resolve ( baseUri, ( String ) value ) );
					}
				} else {
					throw new JsonLdError ( INVALID_BASE_IRI,
					                        "@base must be a string" );
				}
			}

			// 3.5
			if ( ( ( Map<String, Object> ) context ).containsKey ( "@vocab" ) ) {
				const Object value = ( ( Map<String, Object> ) context ).get ( "@vocab" );
				if ( value == null )
					result.remove ( "@vocab" ); else if ( value.isString() ) {
					if ( JsonLdUtils.isAbsoluteIri ( ( String ) value ) )
						result.put ( "@vocab", value ); else {
						throw new JsonLdError ( Error.INVALID_VOCAB_MAPPING,
						                        "@value must be an absolute IRI" );
					}
				} else {
					throw new JsonLdError ( Error.INVALID_VOCAB_MAPPING,
					                        "@vocab must be a string or null" );
				}
			}

			// 3.6
			if ( ( ( Map<String, Object> ) context ).containsKey ( "@language" ) ) {
				const Object value = ( ( Map<String, Object> ) context ).get ( "@language" );
				if ( value == null )
					result.remove ( "@language" ); else if ( value.isString() )
					result.put ( "@language", ( ( String ) value ).toLowerCase() ); else
					throw new JsonLdError ( Error.INVALID_DEFAULT_LANGUAGE, value );
			}

			// 3.7
			const Map<String, Boolean> defined = new LinkedHashMap<String, Boolean>();
			for ( const String key : ( ( Map<String, Object> ) context ).keySet() ) {
				if ( "@base".equals ( key ) || "@vocab".equals ( key ) || "@language".equals ( key ) )
					continue;
				result.createTermDefinition ( ( Map<String, Object> ) context, key, defined );
			}
		}
		return result;
	}

	void createTermDefinition ( Map<String, Object> context, String term,
	                            Map<String, Boolean> defined )  {
		if ( defined.containsKey ( term ) ) {
			if ( Boolean.TRUE.equals ( defined.get ( term ) ) )
				return;
			throw new JsonLdError ( Error.CYCLIC_IRI_MAPPING, term );
		}

		defined.put ( term, false );

		if ( JsonLdUtils.isKeyword ( term ) )
			throw new JsonLdError ( Error.KEYWORD_REDEFINITION, term );

		termDefinitions.remove ( term );
		Object value = context.get ( term );
		if ( value == null
		        || ( value.isMap() && ( ( Map<String, Object> ) value ).containsKey ( "@id" ) && ( ( Map<String, Object> ) value )
		             .get ( "@id" ) == null ) ) {
			termDefinitions.put ( term, null );
			defined.put ( term, true );
			return;
		}

		if ( value.isString() )
			value = newMap ( "@id", value );

		if ( ! ( value.isMap() ) )
			throw new JsonLdError ( Error.INVALID_TERM_DEFINITION, value );

		// casting the value so it doesn't have to be done below everytime
		const Map<String, Object> val = ( Map<String, Object> ) value;

		// 9) create a new term definition
		const Map<String, Object> definition = newMap();

		// 10)
		if ( val.containsKey ( "@type" ) ) {
			if ( ! ( val.get ( "@type" ).isString() ) )
				throw new JsonLdError ( Error.INVALID_TYPE_MAPPING, val.get ( "@type" ) );
			String type = ( String ) val.get ( "@type" );
			try {
				type = expandIri ( ( String ) val.get ( "@type" ), false, true, context, defined );
			} catch ( const JsonLdError error ) {
				if ( error.getType() != Error.INVALID_IRI_MAPPING )
					throw error;
				throw new JsonLdError ( Error.INVALID_TYPE_MAPPING, type );
			}
			// TODO: fix check for absoluteIri (blank nodes shouldn't count, at
			// least not here!)
			if ( "@id".equals ( type ) || "@vocab".equals ( type )
			        || ( !type.startsWith ( "_:" ) && JsonLdUtils.isAbsoluteIri ( type ) ) )
				definition.put ( "@type", type ); else
				throw new JsonLdError ( Error.INVALID_TYPE_MAPPING, type );
		}

		// 11)
		if ( val.containsKey ( "@reverse" ) ) {
			if ( val.containsKey ( "@id" ) )
				throw new JsonLdError ( Error.INVALID_REVERSE_PROPERTY, val );
			if ( ! ( val.get ( "@reverse" ).isString() ) ) {
				throw new JsonLdError ( Error.INVALID_IRI_MAPPING,
				                        "Expected String for @reverse value. got "
				                        + ( val.get ( "@reverse" ) == null ? "null" : val.get ( "@reverse" )
				                            .getClass() ) );
			}
			const String reverse = expandIri ( val.get ( "@reverse" ).str(), false, true,
			                                   context, defined );
			if ( !JsonLdUtils.isAbsoluteIri ( reverse ) ) {
				throw new JsonLdError ( Error.INVALID_IRI_MAPPING, "Non-absolute @reverse IRI: "
				                        + reverse );
			}
			definition.put ( "@id", reverse );
			if ( val.containsKey ( "@container" ) ) {
				const String container = ( String ) val.get ( "@container" );
				if ( container == null || "@set".equals ( container ) || "@index".equals ( container ) )
					definition.put ( "@container", container ); else {
					throw new JsonLdError ( Error.INVALID_REVERSE_PROPERTY,
					                        "reverse properties only support set- and index-containers" );
				}
			}
			definition.put ( "@reverse", true );
			termDefinitions.put ( term, definition );
			defined.put ( term, true );
			return;
		}

		// 12)
		definition.put ( "@reverse", false );

		// 13)
		if ( val.get ( "@id" ) != null && !term.equals ( val.get ( "@id" ) ) ) {
			if ( ! ( val.get ( "@id" ).isString() ) ) {
				throw new JsonLdError ( Error.INVALID_IRI_MAPPING,
				                        "expected value of @id to be a string" );
			}

			const String res = expandIri ( ( String ) val.get ( "@id" ), false, true, context,
			                               defined );
			if ( JsonLdUtils.isKeyword ( res ) || JsonLdUtils.isAbsoluteIri ( res ) ) {
				if ( "@context".equals ( res ) )
					throw new JsonLdError ( Error.INVALID_KEYWORD_ALIAS, "cannot alias @context" );
				definition.put ( "@id", res );
			} else {
				throw new JsonLdError ( Error.INVALID_IRI_MAPPING,
				                        "resulting IRI mapping should be a keyword, absolute IRI or blank node" );
			}
		}

		// 14)
		else if ( term.indexOf ( ":" ) >= 0 ) {
			const int colIndex = term.indexOf ( ":" );
			const String prefix = term.substring ( 0, colIndex );
			const String suffix = term.substring ( colIndex + 1 );
			if ( context.containsKey ( prefix ) )
				createTermDefinition ( context, prefix, defined );
			if ( termDefinitions.containsKey ( prefix ) ) {
				definition.put ( "@id",
				                 ( ( Map<String, Object> ) termDefinitions.get ( prefix ) ).get ( "@id" ) + suffix );
			} else
				definition.put ( "@id", term );
			// 15)
		} else if ( containsKey ( "@vocab" ) )
			definition.put ( "@id", get ( "@vocab" ) + term ); else {
			throw new JsonLdError ( Error.INVALID_IRI_MAPPING,
			                        "relative term definition without vocab mapping" );
		}

		// 16)
		if ( val.containsKey ( "@container" ) ) {
			const String container = ( String ) val.get ( "@container" );
			if ( !"@list".equals ( container ) && !"@set".equals ( container )
			        && !"@index".equals ( container ) && !"@language".equals ( container ) ) {
				throw new JsonLdError ( Error.INVALID_CONTAINER_MAPPING,
				                        "@container must be either @list, @set, @index, or @language" );
			}
			definition.put ( "@container", container );
		}

		// 17)
		if ( val.containsKey ( "@language" ) && !val.containsKey ( "@type" ) ) {
			if ( val.get ( "@language" ) == null || val.get ( "@language" ).isString() ) {
				const String language = ( String ) val.get ( "@language" );
				definition.put ( "@language", language != null ? language.toLowerCase() : null );
			} else {
				throw new JsonLdError ( Error.INVALID_LANGUAGE_MAPPING,
				                        "@language must be a string or null" );
			}
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
	String expandIri ( String value, boolean relative, boolean vocab, Map<String, Object> context,
	                   Map<String, Boolean> defined )  {
		// 1)
		if ( value == null || JsonLdUtils.isKeyword ( value ) )
			return value;
		// 2)
		if ( context != null && context.containsKey ( value )
		        && !Boolean.TRUE.equals ( defined.get ( value ) ) )
			createTermDefinition ( context, value, defined );
		// 3)
		if ( vocab && termDefinitions.containsKey ( value ) ) {
			const Map<String, Object> td = ( LinkedHashMap<String, Object> ) termDefinitions
			                               .get ( value );
			if ( td != null )
				return ( String ) td.get ( "@id" ); else
				return null;
		}
		// 4)
		const int colIndex = value.indexOf ( ":" );
		if ( colIndex >= 0 ) {
			// 4.1)
			const String prefix = value.substring ( 0, colIndex );
			const String suffix = value.substring ( colIndex + 1 );
			// 4.2)
			if ( "_".equals ( prefix ) || suffix.startsWith ( "//" ) )
				return value;
			// 4.3)
			if ( context != null && context.containsKey ( prefix )
			        && ( !defined.containsKey ( prefix ) || defined.get ( prefix ) == false ) )
				createTermDefinition ( context, prefix, defined );
			// 4.4)
			if ( termDefinitions.containsKey ( prefix ) ) {
				return ( String ) ( ( LinkedHashMap<String, Object> ) termDefinitions.get ( prefix ) )
				       .get ( "@id" ) + suffix;
			}
			// 4.5)
			return value;
		}
		// 5)
		if ( vocab && containsKey ( "@vocab" ) )
			return get ( "@vocab" ) + value;
		// 6)
		else if ( relative )
			return JsonLdUrl.resolve ( ( String ) get ( "@base" ), value ); else if ( context != null && JsonLdUtils.isRelativeIri ( value ) )
			throw new JsonLdError ( Error.INVALID_IRI_MAPPING, "not an absolute IRI: " + value );
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
		// 1)
		if ( iri == null )
			return null;

		// 2)
		if ( relativeToVocab && getInverse().containsKey ( iri ) ) {
			// 2.1)
			String defaultLanguage = ( String ) get ( "@language" );
			if ( defaultLanguage == null )
				defaultLanguage = "@none";

			// 2.2)
			const List<String> containers = new ArrayList<String>();
			// 2.3)
			String typeLanguage = "@language";
			String typeLanguageValue = "@null";

			// 2.4)
			if ( value.isMap() && ( ( Map<String, Object> ) value ).containsKey ( "@index" ) )
				containers.push_back ( "@index" );

			// 2.5)
			if ( reverse ) {
				typeLanguage = "@type";
				typeLanguageValue = "@reverse";
				containers.push_back ( "@set" );
			}
			// 2.6)
			else if ( value.isMap() && ( ( Map<String, Object> ) value ).containsKey ( "@list" ) ) {
				// 2.6.1)
				if ( ! ( ( Map<String, Object> ) value ).containsKey ( "@index" ) )
					containers.push_back ( "@list" );
				// 2.6.2)
				const List<Object> list = ( List<Object> ) ( ( Map<String, Object> ) value ).get ( "@list" );
				// 2.6.3)
				String commonLanguage = ( list.size() == 0 ) ? defaultLanguage : null;
				String commonType = null;
				// 2.6.4)
				for ( const Object item : list ) {
					// 2.6.4.1)
					String itemLanguage = "@none";
					String itemType = "@none";
					// 2.6.4.2)
					if ( JsonLdUtils.isValue ( item ) ) {
						// 2.6.4.2.1)
						if ( ( ( Map<String, Object> ) item ).containsKey ( "@language" ) )
							itemLanguage = ( String ) ( ( Map<String, Object> ) item ).get ( "@language" );
						// 2.6.4.2.2)
						else if ( ( ( Map<String, Object> ) item ).containsKey ( "@type" ) )
							itemType = ( String ) ( ( Map<String, Object> ) item ).get ( "@type" );
						// 2.6.4.2.3)
						else
							itemLanguage = "@null";
					}
					// 2.6.4.3)
					else
						itemType = "@id";
					// 2.6.4.4)
					if ( commonLanguage == null )
						commonLanguage = itemLanguage;
					// 2.6.4.5)
					else if ( !commonLanguage.equals ( itemLanguage ) && JsonLdUtils.isValue ( item ) )
						commonLanguage = "@none";
					// 2.6.4.6)
					if ( commonType == null )
						commonType = itemType;
					// 2.6.4.7)
					else if ( !commonType.equals ( itemType ) )
						commonType = "@none";
					// 2.6.4.8)
					if ( "@none".equals ( commonLanguage ) && "@none".equals ( commonType ) )
						break;
				}
				// 2.6.5)
				commonLanguage = ( commonLanguage != null ) ? commonLanguage : "@none";
				// 2.6.6)
				commonType = ( commonType != null ) ? commonType : "@none";
				// 2.6.7)
				if ( !"@none".equals ( commonType ) ) {
					typeLanguage = "@type";
					typeLanguageValue = commonType;
				}
				// 2.6.8)
				else
					typeLanguageValue = commonLanguage;
			}
			// 2.7)
			else {
				// 2.7.1)
				if ( value.isMap() && ( ( Map<String, Object> ) value ).containsKey ( "@value" ) ) {
					// 2.7.1.1)
					if ( ( ( Map<String, Object> ) value ).containsKey ( "@language" )
					        && ! ( ( Map<String, Object> ) value ).containsKey ( "@index" ) ) {
						containers.push_back ( "@language" );
						typeLanguageValue = ( String ) ( ( Map<String, Object> ) value ).get ( "@language" );
					}
					// 2.7.1.2)
					else if ( ( ( Map<String, Object> ) value ).containsKey ( "@type" ) ) {
						typeLanguage = "@type";
						typeLanguageValue = ( String ) ( ( Map<String, Object> ) value ).get ( "@type" );
					}
				}
				// 2.7.2)
				else {
					typeLanguage = "@type";
					typeLanguageValue = "@id";
				}
				// 2.7.3)
				containers.push_back ( "@set" );
			}

			// 2.8)
			containers.push_back ( "@none" );
			// 2.9)
			if ( typeLanguageValue == null )
				typeLanguageValue = "@null";
			// 2.10)
			const List<String> preferredValues = new ArrayList<String>();
			// 2.11)
			if ( "@reverse".equals ( typeLanguageValue ) )
				preferredValues.push_back ( "@reverse" );
			// 2.12)
			if ( ( "@reverse".equals ( typeLanguageValue ) || "@id".equals ( typeLanguageValue ) )
			&& ( value.isMap() ) && ( value.map().containsKey ( "@id" ) ) {
			// 2.12.1)
			const String result = compactIri (
			                          value.map.get ( "@id" ).str(), null, true, true );
				if ( termDefinitions.containsKey ( result )
				        && termDefinitions.map().get ( result ) .map().containsKey ( "@id" )
				        && value.map() .get ( "@id" ).equals (
				            termDefinitions.map().get ( result ).map() .get ( "@id" ) ) ) {
					preferredValues.push_back ( "@vocab" );
					preferredValues.push_back ( "@id" );
				}
				// 2.12.2)
				else {
					preferredValues.push_back ( "@id" );
					preferredValues.push_back ( "@vocab" );
				}
			}
			// 2.13)
			else
				preferredValues.push_back ( typeLanguageValue );
				preferredValues.push_back ( "@none" );

				// 2.14)
				const String term = selectTerm ( iri, containers, typeLanguage, preferredValues );
				                    // 2.15)
				                    if ( term != null )
				                    return term;
			}

	// 3)
	if ( relativeToVocab && containsKey ( "@vocab" ) ) {
			// determine if vocab is a prefix of the iri
			const String vocab = ( String ) get ( "@vocab" );
			// 3.1)
			if ( iri.indexOf ( vocab ) == 0 && !iri.equals ( vocab ) ) {
				// use suffix as relative iri if it is not a term in the
				// active context
				const String suffix = iri.substring ( vocab.length() );
				if ( !termDefinitions.containsKey ( suffix ) )
					return suffix;
			}
		}

		// 4)
		String compactIRI = null;
		// 5)
		for ( const String term : termDefinitions.keySet() ) {
			Map<String, Object> termDefinition = termDefinitions map().get ( term ).map();
			// 5.1)
			if ( term.contains ( ":" ) ) continue;
			// 5.2)
			if ( termDefinition == null || iri.equals ( termDefinition.get ( "@id" ) )
			        || !iri.startsWith ( ( String ) termDefinition.get ( "@id" ) ) )
				continue;
			// 5.3)
			const String candidate = term + ":"
			                         + iri.substring ( ( ( String ) termDefinition.get ( "@id" ) ).length() );
			// 5.4)
			if ( ( compactIRI == null || compareShortestLeast ( candidate, compactIRI ) < 0 )
			        && ( !termDefinitions.containsKey ( candidate ) || ( iri
			                .equals ( ( ( Map<String, Object> ) termDefinitions.get ( candidate ) )
			                          .get ( "@id" ) ) && value == null ) ) )
				compactIRI = candidate;

		}
		// 6)
		if ( compactIRI != null ) return compactIRI;
		// 7)
		if ( !relativeToVocab ) return JsonLdUrl.removeBase ( get ( "@base" ), iri );
		// 8)
		return iri;
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
		const Map<String, String> prefixes = new LinkedHashMap<String, String>();
		for ( const String term : termDefinitions.keySet() ) {
			if ( term.contains ( ":" ) ) continue;
			const Map<String, Object> termDefinition = ( Map<String, Object> ) termDefinitions .get ( term );
			if ( termDefinition == null ) continue;
			const String id = ( String ) termDefinition.get ( "@id" );
			if ( id == null ) continue;
			if ( term.startsWith ( "@" ) || id.startsWith ( "@" ) ) continue;
			if ( !onlyCommonPrefixes || id.endsWith ( "/" ) || id.endsWith ( "#" ) ) prefixes.put ( term, id );
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
		if ( inverse != null )
			return inverse;

		// 1)
		inverse = newMap();

		// 2)
		String defaultLanguage = ( String ) get ( "@language" );
		if ( defaultLanguage == null )
			defaultLanguage = "@none";

		// create term selections for each mapping in the context, ordererd by
		// shortest and then lexicographically least
		List<String> terms ( termDefinitions.keySet() );
		Collections.sort ( terms, new Comparator<String>() {
			virtual int compare ( String a, String b ) {
				return compareShortestLeast ( a, b );
			}
		} );

		for ( String term : terms ) {
			Map<String, Object> definition = ( termDefinitions.get ( term ).obj();
			                                   // 3.1)
			                                   if ( definition == null )
			                                   continue;

			                                   // 3.2)
			                                   String container = ( String ) definition.get ( "@container" );
			                                   if ( container == null )
				                                   container = "@none";

				                                   // 3.3)
				                                   const String iri = ( String ) definition.get ( "@id" );

				                                   // 3.4 + 3.5)
				                                   Map<String, Object> containerMap = ( Map<String, Object> ) inverse.get ( iri );
				if ( containerMap == null ) {
					containerMap = newMap();
						inverse.put ( iri, containerMap );
					}

			// 3.6 + 3.7)
			Map<String, Object> typeLanguageMap = ( Map<String, Object> ) containerMap.get ( container );
			if ( typeLanguageMap == null ) {
			typeLanguageMap = newMap();
				typeLanguageMap.put ( "@language", newMap() );
				typeLanguageMap.put ( "@type", newMap() );
				containerMap.put ( container, typeLanguageMap );
			}

			// 3.8)
			if ( Boolean.TRUE.equals ( definition.get ( "@reverse" ) ) ) {
			const Map<String, Object> typeMap = ( Map<String, Object> ) typeLanguageMap
				                                    .get ( "@type" );
				if ( !typeMap.containsKey ( "@reverse" ) )
					typeMap.put ( "@reverse", term );
				// 3.9)
			} else if ( definition.containsKey ( "@type" ) ) {
			const Map<String, Object> typeMap = ( Map<String, Object> ) typeLanguageMap
				                                    .get ( "@type" );
				if ( !typeMap.containsKey ( definition.get ( "@type" ) ) )
					typeMap.put ( ( String ) definition.get ( "@type" ), term );
				// 3.10)
			} else if ( definition.containsKey ( "@language" ) ) {
			const Map<String, Object> languageMap = ( Map<String, Object> ) typeLanguageMap
				                                        .get ( "@language" );
				String language = ( String ) definition.get ( "@language" );
				if ( language == null )
					language = "@null";
				if ( !languageMap.containsKey ( language ) )
					languageMap.put ( language, term );
				// 3.11)
			} else {
				// 3.11.1)
				const Map<String, Object> languageMap = ( Map<String, Object> ) typeLanguageMap
				                                        .get ( "@language" );
				// 3.11.2)
				if ( !languageMap.containsKey ( "@language" ) )
					languageMap.put ( "@language", term );
				// 3.11.3)
				if ( !languageMap.containsKey ( "@none" ) )
					languageMap.put ( "@none", term );
				// 3.11.4)
				const Map<String, Object> typeMap = ( Map<String, Object> ) typeLanguageMap
				                                    .get ( "@type" );
				// 3.11.5)
				if ( !typeMap.containsKey ( "@none" ) )
					typeMap.put ( "@none", term );
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
	String selectTerm ( String iri, List<String> containers, String typeLanguage,
	                    List<String> preferredValues ) {
		const Map<String, Object> inv = getInverse();
		// 1)
		const Map<String, Object> containerMap = ( Map<String, Object> ) inv.get ( iri );
		// 2)
		for ( const String container : containers ) {
			// 2.1)
			if ( !containerMap.containsKey ( container ) )
				continue;
			// 2.2)
			const Map<String, Object> typeLanguageMap = ( Map<String, Object> ) containerMap .get ( container );
			// 2.3)
			const Map<String, Object> valueMap = ( Map<String, Object> ) typeLanguageMap .get ( typeLanguage );
			// 2.4 )
			for ( const String item : preferredValues ) {
				// 2.4.1
				if ( !valueMap.containsKey ( item ) ) continue;
				// 2.4.2
				return ( String ) valueMap.get ( item );
			}
		}
		// 3)
		return null;
	}

	/**
	    Retrieve container mapping.

	    @param property
	              The Property to get a container mapping for.
	    @return The container mapping
	*/
	Object expandValue ( String activeProperty, Object value )  {
		const Map<String, Object> rval = newMap();
		const Map<String, Object> td = getTermDefinition ( activeProperty );
		// 1)
		if ( td != null && "@id".equals ( td.get ( "@type" ) ) ) {
			// TODO: i'm pretty sure value should be a string if the @type is
			// @id
			rval.put ( "@id", expandIri ( value.toString(), true, false, null, null ) );
			return rval;
		}
		// 2)
		if ( td != null && "@vocab".equals ( td.get ( "@type" ) ) ) {
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
				const String lang = ( String ) td.get ( "@language" );
				if ( lang != null ) rval.put ( "@language", lang );
			}
			// 5.2)
			else if ( get ( "@language" ) != null ) rval.put ( "@language", get ( "@language" ) );
		}
		return rval;
	}

	Map<String, Object> serialize() {
		const Map<String, Object> ctx = newMap();
		if ( get ( "@base" ) != null && !get ( "@base" ).equals ( options.getBase() ) ) ctx.put ( "@base", get ( "@base" ) );
		if ( get ( "@language" ) != null ) ctx.put ( "@language", get ( "@language" ) );
		if ( get ( "@vocab" ) != null ) ctx.put ( "@vocab", get ( "@vocab" ) );
		for ( const String term : termDefinitions.keySet() ) {
			const Map<String, Object> definition = ( Map<String, Object> ) termDefinitions.get ( term );
			if ( definition.get ( "@language" ) == null
			        && definition.get ( "@container" ) == null
			        && definition.get ( "@type" ) == null
			        && ( definition.get ( "@reverse" ) == null || Boolean.FALSE.equals ( definition .get ( "@reverse" ) ) ) ) {
				const String cid = compactIri ( ( String ) definition.get ( "@id" ) );
				ctx.put ( term, term.equals ( cid ) ? definition.get ( "@id" ) : cid );
			} else {
				const Map<String, Object> defn = newMap();
				const String cid = compactIri ( ( String ) definition.get ( "@id" ) );
				const Boolean reverseProperty = Boolean.TRUE.equals ( definition.get ( "@reverse" ) );
				if ( ! ( term.equals ( cid ) && !reverseProperty ) )
					defn.put ( reverseProperty ? "@reverse" : "@id", cid );
				const String typeMapping = ( String ) definition.get ( "@type" );
				if ( typeMapping != null )
					defn.put ( "@type", JsonLdUtils.isKeyword ( typeMapping ) ? typeMapping
					           : compactIri ( typeMapping, true ) );
			}
			if ( definition.get ( "@container" ) != null ) defn.put ( "@container", definition.get ( "@container" ) );
			const Object lang = definition.get ( "@language" ); if ( definition.get ( "@language" ) != null )
				defn.put ( "@language", Boolean.FALSE.equals ( lang ) ? null : lang );
			ctx.put ( term, defn );
		}
	}

	Map<String, Object> rval;// = newMap();
	if ( ! ( ctx == null || ctx.isEmpty() ) ) rval.put ( "@context", ctx );
	return rval;
}

void init ( JsonLdOptions options_ ) {
	options = options_;
	if ( options.getBase().size() ) put ( "@base", options.getBase() );
	termDefinitions = base_t();
}

String getTypeMapping ( String property ) {
	const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
	if ( td == null ) return null;
	return ( String ) td.get ( "@type" );
}

String getLanguageMapping ( String property ) {
	const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
	if ( td == null ) return null;
	return ( String ) td.get ( "@language" );
}

Map<String, Object> getTermDefinition ( String key ) {
	return ( ( Map<String, Object> ) termDefinitions.get ( key ) );
}


String getContainer ( String property ) {
	if ( "@graph".equals ( property ) ) return "@set";
	if ( JsonLdUtils.isKeyword ( property ) ) return property;
	const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
	if ( td == null ) return null;
	return ( String ) td.get ( "@container" );
}

Boolean Cotext::isReverseProperty ( String property ) {
	const Map<String, Object> td = ( Map<String, Object> ) termDefinitions.get ( property );
	if ( td == null ) return false;
	const Object reverse = td.get ( "@reverse" );
	return reverse != null && ( Boolean ) reverse;
}
Object getContextValue ( String activeProperty, String string )  {
	throw JsonLdError ( Error.NOT_IMPLEMENTED,
	                    "getContextValue is only used by old code so far and thus isn't implemented" );
}
};
#endif
