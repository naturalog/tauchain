// package com.github.jsonldjava.utils;

// import java.net.URI;
// import java.net.URISyntaxException;
// import java.util.ArrayList;
// import java.util.Arrays;
// import java.util.List;
// import java.util.regex.Matcher;
// import java.util.regex.Pattern;
#include "defs.h"
/*

#include <boost/variant.hpp>

#include <boost/logic/tribool.hpp>
//BOOST_TRIBOOL_THIRD_STATE ( bnull )

#include <regex>
#include <string>

typedef std::string String;
typedef bool boolean;
typedef boost::tribool Boolean;
template<typename K> using List = std::vector<K>;
*/
/*
List<String> split ( String input, char delim )
//really, c++?
{
	String item;
	List<String> output;
	std::istringstream stream ( input );
	while ( std::getline ( stream, item, delim ) )
		output.push_back ( item );
	return output;
}

bool endsWith ( String s, char e ) {
	return s != "" && s.back() == e;
}
*/
class JsonLdUrl {
public:
	String href;
	String protocol;
	String host;
	String auth;
	String user;
	String password;
	String hostname;
	String port;
	String relative;
	String path;
	String directory;
	String file;
	String query;
	String hash;
	// things not populated by the regex (NOTE: i don't think it matters if
	// these are null or "" to start with) // lets hope it doesnt
	String pathname;
	String normalizedPath;
	String authority;


	void debugPrint() {
		std::cout <<
		          "href = \"" << href << "\"" << std::endl <<
		          "protocol = \"" << protocol << "\"" << std::endl <<
		          "host = \"" << host << "\"" << std::endl <<
		          "auth = \"" << auth << "\"" << std::endl <<
		          "user = \"" << user << "\"" << std::endl <<
		          "password = \"" << password << "\"" << std::endl <<
		          "hostname = \"" << hostname << "\"" << std::endl <<
		          "port = \"" << port << "\"" << std::endl <<
		          "relative = \"" << relative << "\"" << std::endl <<
		          "path = \"" << path << "\"" << std::endl <<
		          "directory = \"" << directory << "\"" << std::endl <<
		          "file = \"" << file << "\"" << std::endl <<
		          "query = \"" << query << "\"" << std::endl <<
		          "hash = \"" << hash << "\"" << std::endl <<
		          "pathname = \"" << pathname << "\"" << std::endl <<
		          "normalizedPath = \"" << normalizedPath << "\"" << std::endl <<
		          "authority = \"" << authority << "\"" << std::endl;
	}

	static JsonLdUrl parse ( String url ) {
		JsonLdUrl rval;
		rval.href = url;
		boost::smatch match;
		try {
			boost::regex parser ( "^(?:([^:\\/?#]+):)?(?:\\/\\/((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\\/?#]*)(?::(\\d*))?))?((((?:[^?#\\/]*\\/)*)([^?#]*))(?:\\?([^#]*))?(?:#(.*))?)" );
			if ( !boost::regex_match ( url, match, parser ) )
				return rval;
		} catch ( const boost::regex_error& e ) {
			std::cout << "regex_error caught: " << e.code() << '\n';
		};

		assert ( match.size() == 14 );

		rval.protocol = match[1].str ();
		rval.host = match[2].str ();
		rval.auth = match[3].str ();
		rval.user = match[4].str ();
		rval.password = match[5].str ();
		rval.hostname = match[6].str ();
		rval.port = match[7].str ();
		rval.relative = match[8].str ();
		rval.path = match[9].str ();
		rval.directory = match[10].str ();
		rval.file = match[11].str ();
		rval.query = match[12].str ();
		rval.hash = match[13].str ();

		// normalize to node.js API
		if ( rval.host.size() && !rval.path.size() )
			rval.path = "/";
		rval.pathname = rval.path;
		parseAuthority ( rval );
		rval.normalizedPath = removeDotSegments ( rval.pathname, rval.authority.size() );
		if ( rval.query.size() )
			rval.path += "?" + rval.query;
		if ( rval.protocol.size() )
			rval.protocol += ":";//simplify the regex?
		if ( rval.hash.size( ) )
			rval.hash = "#" + rval.hash;

		return rval;
	}



	//    Removes dot segments from a JsonLdUrl path.

	//    @param path
	//              the path to remove dot segments from.
	//    @param hasAuthority
	//              true if the JsonLdUrl has an authority, false if not.
	//    @return The URL without the dot segments


	static String removeDotSegments ( String path, boolean hasAuthority ) {
		String rval = "";

		if ( path.find ( "/" ) == 0 )
			rval = "/";


		// RFC 3986 5.2.4 (reworked)
		List<String> input = split ( path, '/' );
		List<String> output;

		for ( int i = 0; i < input.size(); i++ ) {
			//if (( "." == input[i] ) || ( "" == input[i] ) && input.size() - i > 1 ) ) {
			// // input.remove(0);
			//continue;
			//}
			if ( ".." == input[i] ) {
				// input.remove(0);
				if ( hasAuthority
				        || ( output.size() > 0 && ".." != output[ output.size() - 1 ] ) ) {
					// [].pop() doesn't fail, to replicate this we need to check
					// that there is something to remove
					if ( output.size() > 0 )
						output.pop_back(); // ( output.size() - 1 );
				} else
					output.push_back ( ".." );
				continue;
			}
			output.push_back ( input[i] );
			// // input.remove(0);
		}

		if ( output.size() > 0 ) {
			rval += output[0];
			for ( int i = 1; i < output.size(); i++ )
				rval += "/" + output[i];
		}

		return rval;

	}

	/*
	static String removeBase ( String base, String iri ) {
		return removeBase(parse(baseobj), iri);
	}

	static String removeBase ( JsonLdUrl base, String iri ) {
		JsonLdUrl base;

		{
			String *base_as_string;
			// as baseobj we get either a String or a JsonLdUrl or a null
			if ( (base_as_string = boost::get<String> ( &baseobj )) != NULL )
				base = parse ( base_as_string );
			else if ( (base = boost::get<JsonLdUrl> ( baseobj ) ))
				;
			else
				return iri;
		}

		// establish base root
		String root = "";
		if ( base.href.size() )
			root += base.protocol + "//" + base.authority;
		// support network-path reference with empty base
		else if ( iri.find ( "//" ) != 0 )
			root += "//";

		// IRI not relative to base
		if ( iri.find ( root ) != 0 )
			return iri;

		// remove root from IRI and parse remainder
		const JsonLdUrl rel = JsonLdUrl.parse ( iri.substr ( root.length() , iri.length() ) );

		// remove path segments that match
		List<String> baseSegments = split ( base.normalizedPath, "/" );
		if ( endsWith ( base.normalizedPath, '/' ) )
			baseSegments.push_back ( "" );
		List<String> iriSegments = split ( rel.normalizedPath, '/' );
		if ( endsWith ( rel.normalizedPath , '/' )
		        iriSegments.push_back ( "" );

		while ( baseSegments.size() > 0 && iriSegments.size() > 0 ) {
			if ( baseSegments[0] != iriSegments[0] )
					break;
				if ( baseSegments.size() > 0 )
					baseSegments.erase ( 0 );
				if ( iriSegments.size() > 0 )
					iriSegments..erase ( 0 );
			}
		/*
				// use '../' for each non-matching base segment
				String rval = "";
				if ( baseSegments.size() > 0 ) {
				// don't count the last segment if it isn't a path (doesn't end in
				// '/')
				// don't count empty first segment, it means base began with '/'
				if ( !endsWith ( base.normalizedPath, '/' ) || "".equals ( baseSegments.get ( 0 ) ) )
						baseSegments.remove ( baseSegments.size() - 1 );
					for ( int i = 0; i < baseSegments.size(); ++i )
						rval += "../";
				}

				// prepend remaining segments
				if ( iriSegments.size() > 0 )
				rval += iriSegments.get ( 0 );
				        for ( int i = 1; i < iriSegments.size(); i++ )
					        rval += "/" + iriSegments.get ( i );

					        // add query and hash
					        if ( !"".equals ( rel.query ) )
						        rval += "?" + rel.query;
						        if ( !"".equals ( rel.hash ) )
							        rval += rel.hash;

							        if ( "".equals ( rval ) )
								        rval = "./";

		return rval;

				return rval;
		*/
	//	return "";
	//}

	static String resolve ( String baseUri, String pathToResolve ) {
		/*
			// TODO: some input will need to be normalized to perform the expected
			// result with java
			// TODO: we can do this without using java URI!
			if ( baseUri == null )
				return pathToResolve;
			if ( pathToResolve == null || "".equals ( pathToResolve.trim() ) )
				return baseUri;
			try {
				URI uri = new URI ( baseUri );
				// query string parsing
				if ( pathToResolve.startsWith ( "?" ) ) {
					// drop fragment from uri if it has one
					if ( uri.getFragment() != null )
						uri = new URI ( uri.getScheme(), uri.getAuthority(), uri.getPath(), null, null );
					// add query to the end manually (as URI.resolve does it wrong)
					return uri.toString() + pathToResolve;
				}

				uri = uri.resolve ( pathToResolve );
				// java doesn't discard unnecessary dot segments
				String path = uri.getPath();
				if ( path != null )
					path = JsonLdUrl.removeDotSegments ( uri.getPath(), true );
				return new URI ( uri.getScheme(), uri.getAuthority(), path, uri.getQuery(),
				                 uri.getFragment() ).toString();
			} catch ( const URISyntaxException e ) {
				return null;
			}
		*/
		return "";
	}


	//    Parses the authority for the pre-parsed given JsonLdUrl.

	//    @param parsed
	//              the pre-parsed JsonLdUrl.

private: static void parseAuthority ( JsonLdUrl parsed ) {
		/*
				// parse authority for unparsed relative network-path reference
				if ( parsed.href.indexOf ( ":" ) == -1 && parsed.href.indexOf ( "//" ) == 0
				        && "".equals ( parsed.host ) ) {
					// must parse authority from pathname
					parsed.pathname = parsed.pathname.substring ( 2 );
					const int idx = parsed.pathname.indexOf ( "/" );
					if ( idx == -1 ) {
						parsed.authority = parsed.pathname;
						parsed.pathname = "";
					} else {
						parsed.authority = parsed.pathname.substring ( 0, idx );
						parsed.pathname = parsed.pathname.substring ( idx );
					}
				} else {
					// construct authority
					parsed.authority = parsed.host;
					if ( !"".equals ( parsed.auth ) )
						parsed.authority = parsed.auth + "@" + parsed.authority;
				}
		*/
	}
};

