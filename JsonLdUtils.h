// package com.github.jsonldjava.core;

// import static com.github.jsonldjava.utils.Obj.newMap;

// import java.util.ArrayList;
// import java.util.Arrays;
// import java.util.Collection;
// import java.util.Collections;
// import java.util.List;
// import java.util.Map;

// import com.github.jsonldjava.utils.JsonLdUrl;
// import com.github.jsonldjava.utils.Obj;

#include "JsonLdUrl.h"
#include "defs.h"
#include "Context.h"
template<typename Object>
class JsonLdUtils {
	static const int MAX_CONTEXT_URLS = 10;

	/**
	    Returns whether or not the given value is a keyword (or a keyword alias).

	    @param v
	              the value to check.
	    @param [ctx] the active context to check against.

	    @return true if the value is a keyword, false if not.
	*/
	static boolean isKeyword ( Object key ) {
		if ( !isString ( key ) ) return false;
		return "@base"s == ( key.str() ) || "@context"s == ( key.str() ) || "@container"s == ( key.str() )
		       || "@default"s == ( key.str() ) || "@embed"s == ( key.str() ) || "@explicit"s == ( key.str() )
		       || "@graph"s == ( key.str() ) || "@id"s == ( key.str() ) || "@index"s == ( key.str() )
		       || "@language"s == ( key.str() ) || "@list"s == ( key.str() ) || "@omitDefault"s == ( key.str() )
		       || "@reverse"s == ( key.str() ) || "@preserve"s == ( key.str() ) || "@set"s == ( key.str() )
		       || "@type"s == ( key.str() ) || "@value"s == ( key.str() ) || "@vocab"s == ( key.str() );
	}

public: static Boolean deepCompare ( Object v1, Object v2, Boolean listOrderMatters ) {
		if ( v1 == null )
			return v2 == null; else if ( v2 == null )
			return v1 == null; else if ( v1.isMap() && v2.isMap() ) {
			const Map<String, Object> m1 = v1.obj();
			const Map<String, Object> m2 = v2.obj();
			if ( m1.size() != m2.size() )
				return false;
			for ( /*const String key : m1.keySet()*/ auto key : m1 ) {
				if ( !m2.containsKey ( key.first )
				        || !deepCompare ( m1.get ( key.first ), m2.get ( key.first ), listOrderMatters ) )
					return false;
			}
			return true;
		} else if ( v1.isList() && v2.isList() ) {
			const List<Object> l1 = v1.list();
			const List<Object> l2 = v2.list();
			if ( l1.size() != l2.size() ) return false;
			// used to mark members of l2 that we have already matched to avoid
			// matching the same item twice for lists that have duplicates
			boolean* alreadyMatched = new boolean[l2.size()];
			struct onret {
				bool*p;
				onret ( bool* v ) : p ( v ) {} ~onret() {
					delete[]p;
				}
			} onret_ ( alreadyMatched );
			for ( int i = 0; i < l1.size(); i++ ) {
				const Object o1 = l1.at ( i );
				Boolean gotmatch = false;
				if ( listOrderMatters )
					gotmatch = deepCompare ( o1, l2.at ( i ), listOrderMatters ); else {
					for ( int j = 0; j < l2.size(); j++ ) {
						if ( !alreadyMatched[j] && deepCompare ( o1, l2.at ( j ), listOrderMatters ) ) {
							alreadyMatched[j] = true;
							gotmatch = true;
							break;
						}
					}
				}
				if ( !gotmatch ) return false;
			}
			return true;
		} else return v1.equals ( v2 );
	}

	static Boolean deepCompare ( Object v1, Object v2 ) {
		return deepCompare ( v1, v2, false );
	}

	static boolean deepContains ( List<Object> values, Object value ) {
		for ( const Object item : values ) {
			if ( deepCompare ( item, value, false ) )
				return true;
		}
		return false;
	}

	static void mergeValue ( Map<String, Object>& obj, String& key, Object& value ) {
		if ( !obj.size/*isNull*/() ) return;
		List<Object> values = obj.get ( key ).list();
		if ( !values.size() /* == null */ ) {
			values = /*new*/ ArrayList<Object>();
			obj.put ( key, values );
		}
		if ( "@list"s == ( key )
		        || ( value.isMap() && value.obj().containsKey ( "@list" ) )
		        || !deepContains ( values, value ) )
			values.push_back ( value );
	}

	static void mergeCompactedValue ( Map<String, Object>& obj, String& key, Object& value ) {
		if ( !obj.size() /* == null */ ) return;
		Object prop = obj.get ( key );
		if ( prop.is_null() ) {
			obj.put ( key, value );
			return;
		}
		if ( ! ( prop.isList() ) ) {
			List<Object> tmp = /*new*/ ArrayList<Object>();
			tmp.push_back ( prop );
		}
		if ( value.isList() )
			prop .list().insert ( value.list().begin(), value.list().end(), prop.list().end() );
		else	prop .list().push_back ( value );
	}

	static boolean isAbsoluteIri ( String value ) {
		// TODO: this is a bit simplistic!
		return value.find ( ':' ) != String::npos;
	}

	/**
	    Returns true if the given value is a subject with properties.

	    @param v
	              the value to check.

	    @return true if the value is a subject with properties, false if not.
	*/
	static boolean isNode ( Object v ) {
		// Note: A value is a subject if all of these hold true:
		// 1. It is an Object.
		// 2. It is not a @value, @set, or @list.
		// 3. It has more than 1 key OR any existing key is not @id.
		if ( v.isMap()
		        && ! ( v.obj().containsKey ( "@value" ) || v.obj().containsKey ( "@set" ) || v.obj()
		               .containsKey ( "@list" ) ) )
			return v.obj( ).size() > 1 || !v.obj( ).containsKey ( "@id" );
		return false;
	}

	/**
	    Returns true if the given value is a subject reference.

	    @param v
	              the value to check.

	    @return true if the value is a subject reference, false if not.
	*/
	static boolean isNodeReference ( Object v ) {
		// Note: A value is a subject reference if all of these hold true:
		// 1. It is an Object.
		// 2. It has a single key: @id.
		return v.isMap() && v.obj().size() == 1 &&  v.obj() .containsKey ( "@id" ) ;
	}

	// TODO: fix this test
	static boolean isRelativeIri ( String value ) {
		return ! ( isKeyword ( value ) || isAbsoluteIri ( value ) );
	}
	// //////////////////////////////////////////////////// OLD CODE BELOW

	/**
	    Adds a value to a subject. If the value is an array, all values in the
	    array will be added.

	    Note: If the value is a subject that already exists as a property of the
	    given subject, this method makes no attempt to deeply merge properties.
	    Instead, the value will not be added.

	    @param subject
	              the subject to add the value to.
	    @param property
	              the property that relates the value to the subject.
	    @param value
	              the value to add.
	    @param [propertyIsArray] true if the property is always an array, false
	          if not (default: false).
	    @param [allowDuplicate] true if the property is a @list, false if not
	          (default: false).
	*/
	static void addValue ( Map<String, Object>& subject, String property, Object& value,
	                       boolean propertyIsArray, boolean allowDuplicate ) {

		if ( isArray ( value ) ) {
			if ( value.list().size() == 0 && propertyIsArray && !subject.containsKey ( property ) )
				subject.put ( property, new ArrayList<Object>() );
			for ( Object val : value.list() )
				addValue ( subject, property, val, propertyIsArray, allowDuplicate );
		} else if ( subject.containsKey ( property ) ) {
			// check if subject already has the value if duplicates not allowed
			const boolean hasValue_ = !allowDuplicate && hasValue ( subject, property, value );

			// make property an array if value not present or always an array
			if ( !isArray ( subject.get ( property ) ) && ( !hasValue_ || propertyIsArray ) ) {
				List<Object> tmp = /*new*/ ArrayList<Object>();
				tmp.push_back ( subject.get ( property ) );
				subject.put ( property, tmp );
			}

			// add new value
			if ( !hasValue_ )
				subject.get ( property ).list().push_back ( value );
		} else {
			// add new value as a set or single value
			Object tmp;
			if ( propertyIsArray ) {
				tmp = ArrayList<Object>();
				tmp.list().push_back ( value );
			} else
				tmp = value;
			subject.put ( property, tmp );
		}
	}

	static void addValue ( Map<String, Object> subject, String property, Object value,
	                       boolean propertyIsArray ) {
		addValue ( subject, property, value, propertyIsArray, true );
	}

	static void addValue ( Map<String, Object> subject, String property, Object value ) {
		addValue ( subject, property, value, false, true );
	}

	/**
	    Prepends a base IRI to the given relative IRI.

	    @param base
	              the base IRI.
	    @param iri
	              the relative IRI.

	    @return the absolute IRI.

	           TODO: the JsonLdUrl class isn't as forgiving as the Node.js url
	           parser, we may need to re-implement the parser here to support
	           the flexibility required
	*/
	/*  private: static String prependBase ( Object baseobj, String iri ) {
			// already an absolute IRI
			if ( iri.find ( ":" ) != String::npos )
				return iri;

			// parse base if it is a string
			JsonLdUrl base;
			if ( baseobj.isString() )
				base = JsonLdUrl::parse ( baseobj.str() );
			else
				// assume base is already a JsonLdUrl
				base = ( JsonLdUrl ) baseobj;

			const JsonLdUrl rel = JsonLdUrl.parse ( iri );

			// start hierarchical part
			String hierPart = base.protocol;
			if ( !""s == ( rel.authority ) )
				hierPart += "//" + rel.authority; else if ( !""s == ( base.href ) )
				hierPart += "//" + base.authority;

			// per RFC3986 normalize
			String path;

			// IRI represents an absolute path
			if ( rel.pathname.indexOf ( "/" ) == 0 )
				path = rel.pathname; else {
				path = base.pathname;

				// append relative path to the end of the last directory from base
				if ( !""s == ( rel.pathname ) ) {
					path = path.substring ( 0, path.lastIndexOf ( "/" ) + 1 );
					if ( path.length() > 0 && !path.endsWith ( "/" ) )
						path += "/";
					path += rel.pathname;
				}
			}

			// remove slashes anddots in path
			path = JsonLdUrl.removeDotSegments ( path, !""s == ( hierPart ) );

			// add query and hash
			if ( !""s == ( rel.query ) )
				path += "?" + rel.query;

			if ( !""s == ( rel.hash ) )
				path += rel.hash;

			const String rval = hierPart + path;

			if ( ""s == ( rval ) )
				return "./";
			return rval;
		}
	*/
	/**
	    Expands a language map.

	    @param languageMap
	              the language map to expand.

	    @return the expanded language map.
	    @
	*/
	static List<Object> expandLanguageMap ( Map<String, Object> languageMap )  {
		List<Object> rval;
		//		List<String> keys = new ArrayList<String> ( languageMap.keySet() );
		//		Collections.sort ( keys ); // lexicographically sort languages
		//		for ( const String key : keys ) {
		for ( auto x : languageMap ) {
			String key = x.first;
			List<Object> val;
			if ( !isArray ( languageMap.get ( key ) ) )
				val.push_back ( languageMap.get ( key ) );
			else
				val = languageMap.get ( key ).list();
			for ( const Object item : val ) {
				if ( !isString ( item ) ) throw JsonLdError ( SYNTAX_ERROR );
				Map<String, Object> tmp;
				tmp.put ( "@value", item );
				tmp.put ( "@language", lower ( key ) );
				rval.push_back ( tmp );
			}
		}

		return rval;
	}

	/**
	    Throws an exception if the given value is not a valid @type value.

	    @param v
	              the value to check.
	    @
	*/
	static boolean validateTypeValue ( Object v )  {
		if ( v == null )
			throw NullPointerException ( "\"@type\" value cannot be null" );

		// must be a string, subject reference, or empty object
		if ( v.isString() || ( v.isMap() && ( v.obj().containsKey ( "@id" ) || ( v.obj() .size() == 0 ) ) ) )
			return true;
		// must be an array
		boolean isValid = false;
		if ( v.isList() ) {
			isValid = true;
			for ( const Object i : v.list() ) {
				if ( ! ( i.isString() || ( i.isMap()
				                           &&  i.obj( ).containsKey ( "@id" ) ) ) ) {
					isValid = false;
					break;
				}
			}
		}

		if ( !isValid )
			throw JsonLdError ( SYNTAX_ERROR );
		return true;
	}

	/**
	    Removes a base IRI from the given absolute IRI.

	    @param base
	              the base IRI.
	    @param iri
	              the absolute IRI.

	    @return the relative IRI if relative to base, otherwise the absolute IRI.
	*/
private: static String removeBase ( boost::variant<String, JsonLdUrl>& baseobj, String& iri ) {

		//hey cool, this is in JsonLdUrl.h too

		JsonLdUrl base;
		if ( boost::get<String> ( &baseobj ) ) base = JsonLdUrl::parse ( boost::get<String> ( baseobj ) );
		else base = boost::get<JsonLdUrl> ( baseobj );

		// establish base root
		String root = "";
		if ( ""s != base.href ) root += ( base.protocol ) + "//" + base.authority;
		// support network-path reference with empty base
		else if ( iri.find ( "//" ) ) root += "//";
		// IRI not relative to base
		if ( iri.find ( root ) ) return iri;
		// remove root from IRI and parse remainder
		JsonLdUrl rel = JsonLdUrl::parse ( iri.substr ( root.length() ) );
		// remove path segments that match
		List<String> baseSegments = _split ( base.normalizedPath, "/" );
		List<String> iriSegments = _split ( rel.normalizedPath, "/" );

		while ( baseSegments.size() > 0 && iriSegments.size() > 0 ) {
			if ( baseSegments.at ( 0 ) != iriSegments.at ( 0 ) ) break;
			if ( baseSegments.size() > 0 ) baseSegments.erase ( baseSegments.begin() );
			if ( iriSegments.size() > 0 ) iriSegments.erase ( iriSegments.begin() );
		}

		// use '../' for each non-matching base segment
		String rval = "";
		if ( baseSegments.size() > 0 ) {
			// don't count the last segment if it isn't a path (doesn't end in
			// '/')
			// don't count empty first segment, it means base began with '/'
			if ( !endsWith ( base.normalizedPath, "/" ) || ""s == baseSegments.at ( 0 ) )
				baseSegments.erase ( --baseSegments.end() );
			for ( int i = 0; i < baseSegments.size(); ++i ) rval += "../";
		}
		// prepend remaining segments
		rval += _join ( iriSegments, "/" );
		// add query and hash
		if ( ""s != rel.query ) rval += "?" + rel.query;
		if ( ""s != rel.hash ) rval += rel.hash;
		if ( ""s == rval ) rval = "./";
		return rval;
	}


	/**
	    replicate javascript .join because i'm too lazy to keep doing it manually

	    @param iriSegments
	    @param string
	    @return
	*/
private: static String _join ( const List<String>& list, const String& joiner ) {
		String rval = "";
		if ( list.size() > 0 )
			rval += list.at ( 0 );
		for ( int i = 1; i < list.size(); i++ )
			rval += joiner + list.at ( i );
		return rval;
	}

	/**
	    replicates the functionality of javascript .split, which has different
	    results to java's String.split if there is a trailing /

	    @param string
	    @param delim
	    @return
	*/
private:
	template<typename charT>
	static List<String> _split ( String string, charT delim ) {
		List<String> rval = split ( delim );
		if ( endsWith ( string, "/" ) )
			// javascript .split includes a blank entry if the string ends with
			// the delimiter, java .split does not so we need to add it manually
			rval.push_back ( "" );
		return rval;
	}

	/**
	    Compares two strings first based on length and then lexicographically.

	    @param a
	              the first string.
	    @param b
	              the second string.

	    @return -1 if a < b, 1 if a > b, 0 if a == b.
	*/
	static int compareShortestLeast ( String a, String b ) {
		if ( a.length() < b.length() ) return -1;
		else if ( b.length() < a.length() ) return 1;
		return a < b ? -1 : a == b ? 0 : -1;
	}

	/**
	    Determines if the given value is a property of the given subject.

	    @param subject
	              the subject to check.
	    @param property
	              the property to check.
	    @param value
	              the value to check.

	    @return true if the value exists, false if not.
	*/
	static boolean hasValue ( Map<String, Object> subject, String property, Object value ) {
		boolean rval = false;
		if ( hasProperty ( subject, property ) ) {
			Object val = subject.get ( property );
			const boolean isList_ = isList ( val );
			if ( isList_ || val.isList() ) {
				if ( isList_ ) val = val.obj( ).get ( "@list" );
				for ( const Object i : val.list() ) {
					if ( compareValues ( value, i ) ) {
						rval = true;
						break;
					}
				}
			} else if ( ! value.isList() )
				rval = compareValues ( value, val );
		}
		return rval;
	}

	static boolean hasProperty ( Map<String, Object> subject, String property ) {
		boolean rval = false;
		if ( subject.containsKey ( property ) ) {
			Object value = subject.get ( property );
			rval = ( ! ( value.isList() ) || value.list().size() > 0 );
		}
		return rval;
	}

	/**
	    Compares two JSON-LD values for equality. Two JSON-LD values will be
	    considered equal if:

	    1. They are both primitives of the same type and value. 2. They are both @values
	    with the same @value, @type, and @language, OR 3. They both have @ids
	    they are the same.

	    @param v1
	              the first value.
	    @param v2
	              the second value.

	    @return true if v1 and v2 are considered equal, false if not.
	*/
	static boolean compareValues ( Object v1, Object v2 ) {
		if ( v1.equals ( v2 ) )
			return true;

		if ( isValue ( v1 )
		        && isValue ( v2 )
		        && equals ( v1.obj( ).get ( "@value" ),
		                    v2.obj( ).get ( "@value" ) )
		        && equals ( v1.obj( ).get ( "@type" ),
		                    v2.obj( ).get ( "@type" ) )
		        && equals ( v1.obj( ).get ( "@language" ),
		                    v2.obj( ).get ( "@language" ) )
		        && equals ( v1.obj( ).get ( "@index" ),
		                    v2.obj( ).get ( "@index" ) ) )
			return true;

		if ( ( v1.isMap() &&  v1.obj( ).containsKey ( "@id" ) )
		        && ( v2.isMap() && v2.obj( ).containsKey ( "@id" ) )
		        &&  v1.obj( ).get ( "@id" ) == v2.obj( ).get ( "@id" ) )
			return true;

		return false;
	}

	/**
	    Removes a value from a subject.

	    @param subject
	              the subject.
	    @param property
	              the property that relates the value to the subject.
	    @param value
	              the value to remove.
	    @param [options] the options to use: [propertyIsArray] true if the
	          property is always an array, false if not (default: false).
	*/
	static void removeValue ( Map<String, Object>& subject, const String& property, Map<String, Object>& value ) {
		removeValue ( subject, property, value, false );
	}

	static void removeValue ( Map<String, Object>& subject, String property,
	                          Map<String, Object>& value, boolean propertyIsArray ) {
		// filter out value
		List<Object> values;
		if ( subject.get ( property ).isList() ) {
			for ( Object e : subject.get ( property ).list() ) {
				if ( value != e.obj() ) values.push_back ( value );
			}
		} else if ( value != subject.get ( property ).obj() ) values.push_back ( subject.get ( property ) );
		if ( values.size() == 0 ) subject.remove ( property );
		else if ( values.size() == 1 && !propertyIsArray )
			subject.put ( property, values.at ( 0 ) );
		else subject.put ( property, values );
	}

	/**
	    Returns true if the given value is a blank node.

	    @param v
	              the value to check.

	    @return true if the value is a blank node, false if not.
	*/
	static boolean isBlankNode ( Object v ) {
		// Note: A value is a blank node if all of these hold true:
		// 1. It is an Object.
		// 2. If it has an @id key its value begins with '_:'.
		// 3. It has no keys OR is not a @value, @set, or @list.
		if ( v.isMap() ) {
			if ( v.obj( ).containsKey ( "@id" ) )
				return startsWith ( v.obj().get ( "@id" ).str( ), "_:" );
			else {
				return v.obj().size() == 0
				       || ! ( v.obj().containsKey ( "@value" ) || v.obj().containsKey ( "@set" ) || v.obj()
				              .containsKey ( "@list" ) );
			}
		}
		return false;
	}

	/**
	    Finds all @context URLs in the given JSON-LD input.

	    @param input
	              the JSON-LD input.
	    @param urls
	              a map of URLs (url => false/@contexts).
	    @param replace
	              true to replace the URLs in the given input with the
	    @contexts from the urls map, false not to.

	    @return true if new URLs to resolve were found, false if not.
	*/
private: static boolean findContextUrls ( Object& input, Map<String, Object>& urls, Boolean replace ) {
		const int count = urls.size();
		if ( input.isList() ) {
			for ( Object i : input.list() )
				findContextUrls ( i, urls, replace );
			return count < urls.size();
		} else if ( input.isMap() ) {
			//			for ( String key : ( input.obj()).keySet() ) {
			for ( auto x : input.obj() ) {
				String key = x.first;
				if ( "@context"s != key ) {
					findContextUrls ( input.obj().get ( key ), urls, replace );
					continue;
				}

				// get @context
				Object ctx = ( input.obj() ).get ( key );

				// array @context
				if ( ctx.isList() ) {
					int length = ( ctx.list() ).size();
					for ( int i = 0; i < length; i++ ) {
						Object _ctx = ctx.list().at ( i );
						if ( _ctx.isString() ) {
							// replace w/@context if requested
							if ( replace ) {
								_ctx = urls.get ( _ctx.str() );
								if ( _ctx.isList() ) {
									// add flattened context
									ctx.list() .erase ( ctx.list().begin() + i );
									ctx.list().insert ( ctx.list().end(),  _ctx.list().begin(), _ctx.list().end() );
									i +=  _ctx.list() .size();
									length +=   _ctx.list() .size();
								} else ctx.list() [i] =  _ctx ;
							}
							// @context JsonLdUrl found
							else if ( !urls.containsKey ( _ctx.str() ) ) urls.put (  _ctx.str(), false/*Boolean.FALSE*/ );
						}
					}
				}
				// string @context
				else if ( ctx.isString() ) {
					// replace w/@context if requested
					if ( replace )
						input.obj().put ( key, urls.get ( ctx.str() ) );
					// @context JsonLdUrl found
					else if ( !urls.containsKey ( ctx.str() ) )
						urls.put ( ctx.str(), false );
				}
			}
			return ( count < urls.size() );
		}
		return false;
	}

	static Object clone ( Object value ) { // throws
		// CloneNotSupportedException {
		Object rval = null;
		/*		if ( value.isCloneable ) {
					try {
						rval = value.getClass().getMethod ( "clone" ).invoke ( value );
					} catch ( const Exception e ) {
						rval = e;
					}
				}*/
		if ( rval == null/* || rval.isException*/ ) {
			// the object wasn't cloneable, or an error occured
			if ( value == null || value.isString() || value.isNumber() || value.isBoolean() ) {
				// strings numbers and booleans are immutable
				rval = value;
			} else {
				// TODO: making this throw runtime exception so it doesn't have
				// to be caught
				// because simply it should never fail in the case of JSON-LD
				// and means that
				// the input JSON-LD is invalid
				//				throw RuntimeException ( new CloneNotSupportedException (
				//				                             ( rval.isException ? ( ( Exception ) rval ).getMessage() : "" ) ) );
				throw std::runtime_error ( "unknown clone" );
			}
		}
		return rval;
	}

	/**
	    Returns true if the given value is a JSON-LD Array

	    @param v
	              the value to check.
	    @return
	*/
	static Boolean isArray ( Object v ) {
		return ( v.isList() );
	}

	/**
	    Returns true if the given value is a JSON-LD List

	    @param v
	              the value to check.
	    @return
	*/
	static Boolean isList ( Object v ) {
		return ( v.isMap() && v.obj().containsKey ( "@list" ) );
	}

	/**
	    Returns true if the given value is a JSON-LD Object

	    @param v
	              the value to check.
	    @return
	*/
	static Boolean isObject ( Object v ) {
		return ( v.isMap() );
	}

	/**
	    Returns true if the given value is a JSON-LD value

	    @param v
	              the value to check.
	    @return
	*/
	static Boolean isValue ( Object v ) {
		return ( v.isMap() &&  v.obj().containsKey ( "@value" ) );
	}

	/**
	    Returns true if the given value is a JSON-LD string

	    @param v
	              the value to check.
	    @return
	*/
	static Boolean isString ( Object v ) {
		// TODO: should this return true for arrays of strings as well?
		return ( v.isString() );
	}
};
