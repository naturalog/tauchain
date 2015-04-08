// package com.github.jsonldjava.core;

// import static com.github.jsonldjava.core.RDFDatasetUtils.parseNQuads;
// import static com.github.jsonldjava.core.RDFDatasetUtils.toNQuad;

// import java.io.UnsupportedEncodingException;
// import java.security.MessageDigest;
// import java.security.NoSuchAlgorithmException;
// import java.util.ArrayList;
// import java.util.Collection;
// import java.util.Collections;
// import java.util.Comparator;
// import java.util.LinkedHashMap;
// import java.util.List;
// import java.util.Map;

// import com.github.jsonldjava.utils.Obj;

class NormalizeUtils {
	const UniqueNamer namer;
	const Map<String, Object> bnodes;
	const List<Object> quads;
	const JsonLdOptions<Object> options;

public:
	NormalizeUtils ( List<Object> quads, Map<String, Object> bnodes, UniqueNamer namer,
	                 JsonLdOptions<Object> options ) :
		options ( options ),
		quads ( quads ),
		bnodes ( bnodes ),
		namer ( namer ) {
	}

	// generates unique and duplicate hashes for bnodes
	Object hashBlankNodes ( List<String> unnamed_ )  {
		List<String> unnamed = unnamed_ ;
		List<String> nextUnnamed;// = new ArrayList<String>();
		Map<String, List<String>> duplicates;// = new LinkedHashMap<String, List<String>>();
		Map<String, String> unique;// = new LinkedHashMap<String, String>();

		// NOTE: not using the same structure as javascript here to avoid
		// possible stack overflows
		// hash quads for each unnamed bnode
		for ( int hui = 0;; hui++ ) {
			if ( hui == unnamed.size() ) {
				// done, name blank nodes
				Boolean named = false;
				List<String> hashes = new ArrayList<String> ( unique.keySet() );
				Collections.sort ( hashes );
				for ( const String hash : hashes ) {
					const String bnode = unique.get ( hash );
					namer.getName ( bnode );
					named = true;
				}

				// continue to hash bnodes if a bnode was assigned a name
				if ( named ) {
					// this resets the initial variables, so it seems like it
					// has to go on the stack
					// but since this is the end of the function either way, it
					// might not have to
					// hashBlankNodes(unnamed);
					hui = -1;
					unnamed = nextUnnamed;
					nextUnnamed = new ArrayList<String>();
					duplicates = new LinkedHashMap<String, List<String>>();
					unique = new LinkedHashMap<String, String>();
					continue;
				}
				// name the duplicate hash bnods
				else {
					// names duplicate hash bnodes
					// enumerate duplicate hash groups in sorted order
					hashes = new ArrayList<String> ( duplicates.keySet() );
					Collections.sort ( hashes );

					// process each group
					for ( int pgi = 0;; pgi++ ) {
						if ( pgi == hashes.size() ) {
							// done, create JSON-LD array
							// return createArray();
							const List<String> normalized = new ArrayList<String>();

							// Note: At this point all bnodes in the set of RDF
							// quads have been
							// assigned canonical names, which have been stored
							// in the 'namer' object.
							// Here each quad is updated by assigning each of
							// its bnodes its new name
							// via the 'namer' object

							// update bnode names in each quad and serialize
							for ( int cai = 0; cai < quads.size(); ++cai ) {
								const Map<String, Object> quad = ( Map<String, Object> ) quads
								                                 .get ( cai );
								for ( const String attr : new String[] { "subject", "object", "name" } ) {
									if ( quad.containsKey ( attr ) ) {
										const Map<String, Object> qa = ( Map<String, Object> ) quad
										                               .get ( attr );
										if ( qa != null
										        && "blank node".equals ( qa.get ( "type" ) )
										        && ( ( String ) qa.get ( "value" ) ).indexOf ( "_:c14n" ) != 0 ) {
											qa.put ( "value",
											         namer.getName ( ( String ) qa.get ( ( "value" ) ) ) );
										}
									}
								}
								normalized
								.add ( toNQuad (
								           ( RDFDataset.Quad ) quad,
								           quad.containsKey ( "name" )
								           && quad.get ( "name" ) != null ? ( String ) ( ( Map<String, Object> ) quad
								                   .get ( "name" ) ).get ( "value" ) : null ) );
							}

							// sort normalized output
							Collections.sort ( normalized );

							// handle output format
							if ( options.format != null ) {
								if ( "application/nquads".equals ( options.format ) ) {
									String rval = "";
									for ( const String n : normalized )
										rval += n;
									return rval;
								} else {
									throw new JsonLdError ( UNKNOWN_FORMAT,
									                        options.format );
								}
							}
							String rval = "";
							for ( const String n : normalized )
								rval += n;
							return parseNQuads ( rval );
						}

						// name each group member
						const List<String> group = duplicates.get ( hashes.get ( pgi ) );
						const List<HashResult> results = new ArrayList<HashResult>();
						for ( int n = 0;; n++ ) {
							if ( n == group.size() ) {
								// name bnodes in hash order
								std::sort ( results, new Comparator<HashResult>() {
									int compare ( HashResult a, HashResult b ) {
										const int res = a.hash.compareTo ( b.hash );
										return res;
									}
								} );
								for ( const HashResult r : results ) {
									// name all bnodes in path namer in
									// key-entry order
									// Note: key-order is preserved in
									// javascript
									for ( const String key : r.pathNamer.existing().keySet() )
										namer.getName ( key );
								}
								// processGroup(i+1);
								break;
							} else {
								// skip already-named bnodes
								const String bnode = group.get ( n );
								if ( namer.isNamed ( bnode ) )
									continue;

								// hash bnode paths
								const UniqueNamer pathNamer = new UniqueNamer ( "_:b" );
								pathNamer.getName ( bnode );

								const HashResult result = hashPaths ( bnode, bnodes, namer, pathNamer );
								results.add ( result );
							}
						}
					}
				}
			}

			// hash unnamed bnode
			const String bnode = unnamed.get ( hui );
			const String hash = hashQuads ( bnode, bnodes, namer );

			// store hash as unique or a duplicate
			if ( duplicates.containsKey ( hash ) ) {
				duplicates.get ( hash ).add ( bnode );
				nextUnnamed.add ( bnode );
			} else if ( unique.containsKey ( hash ) ) {
				const List<String> tmp = new ArrayList<String>();
				tmp.add ( unique.get ( hash ) );
				tmp.add ( bnode );
				duplicates.put ( hash, tmp );
				nextUnnamed.add ( unique.get ( hash ) );
				nextUnnamed.add ( bnode );
				unique.remove ( hash );
			} else
				unique.put ( hash, bnode );
		}
	}

private:
	class HashResult {
		String hash;
		UniqueNamer pathNamer;
	};

	/**
	    Produces a hash for the paths of adjacent bnodes for a bnode,
	    incorporating all information about its subgraph of bnodes. This method
	    will recursively pick adjacent bnode permutations that produce the
	    lexicographically-least 'path' serializations.

	    @param id
	              the ID of the bnode to hash paths for.
	    @param bnodes
	              the map of bnode quads.
	    @param namer
	              the canonical bnode namer.
	    @param pathNamer
	              the namer used to assign names to adjacent bnodes.
	    @param callback
	              (err, result) called once the operation completes.
	*/
	static HashResult hashPaths ( String id, Map<String, Object> bnodes, UniqueNamer namer,
	                              UniqueNamer pathNamer ) {
		try {
			// create SHA-1 digest
			const MessageDigest md = MessageDigest.getInstance ( "SHA-1" );

			const Map<String, List<String>> groups = new LinkedHashMap<String, List<String>>();
			List<String> groupHashes;
			const List<Object> quads = ( List<Object> ) ( ( Map<String, Object> ) bnodes.get ( id ) )
			                           .get ( "quads" );

			for ( int hpi = 0;; hpi++ ) {
				if ( hpi == quads.size() ) {
					// done , hash groups
					groupHashes = new ArrayList<String> ( groups.keySet() );
					Collections.sort ( groupHashes );
					for ( int hgi = 0;; hgi++ ) {
						if ( hgi == groupHashes.size() ) {
							const HashResult res = new HashResult();
							res.hash = encodeHex ( md.digest() );
							res.pathNamer = pathNamer;
							return res;
						}

						// digest group hash
						const String groupHash = groupHashes.get ( hgi );
						md.update ( groupHash.getBytes ( "UTF-8" ) );

						// choose a path and namer from the permutations
						String chosenPath = null;
						UniqueNamer chosenNamer = null;
						const Permutator permutator = new Permutator ( groups.get ( groupHash ) );
						while ( true ) {
							Boolean contPermutation = false;
							Boolean breakOut = false;
							const List<String> permutation = permutator.next();
							UniqueNamer pathNamerCopy = pathNamer.clone();

							// build adjacent path
							String path = "";
							const List<String> recurse = new ArrayList<String>();
							for ( const String bnode : permutation ) {
								// use canonical name if available
								if ( namer.isNamed ( bnode ) )
									path += namer.getName ( bnode ); else {
									// recurse if bnode isn't named in the path
									// yet
									if ( !pathNamerCopy.isNamed ( bnode ) )
										recurse.add ( bnode );
									path += pathNamerCopy.getName ( bnode );
								}

								// skip permutation if path is already >= chosen
								// path
								if ( chosenPath != null && path.length() >= chosenPath.length()
								        && path.compareTo ( chosenPath ) > 0 ) {
									// return nextPermutation(true);
									if ( permutator.hasNext() )
										contPermutation = true; else {
										// digest chosen path and update namer
										md.update ( chosenPath.getBytes ( "UTF-8" ) );
										pathNamer = chosenNamer;
										// hash the nextGroup
										breakOut = true;
									}
									break;
								}
							}

							// if we should do the next permutation
							if ( contPermutation )
								continue;
							// if we should stop processing this group
							if ( breakOut )
								break;

							// does the next recursion
							for ( int nrn = 0;; nrn++ ) {
								if ( nrn == recurse.size() ) {
									// return nextPermutation(false);
									if ( chosenPath == null || path.compareTo ( chosenPath ) < 0 ) {
										chosenPath = path;
										chosenNamer = pathNamerCopy;
									}
									if ( !permutator.hasNext() ) {
										// digest chosen path and update namer
										md.update ( chosenPath.getBytes ( "UTF-8" ) );
										pathNamer = chosenNamer;
										// hash the nextGroup
										breakOut = true;
									}
									break;
								}

								// do recursion
								const String bnode = recurse.get ( nrn );
								const HashResult result = hashPaths ( bnode, bnodes, namer,
								                                      pathNamerCopy );
								path += pathNamerCopy.getName ( bnode ) + "<" + result.hash + ">";
								pathNamerCopy = result.pathNamer;

								// skip permutation if path is already >= chosen
								// path
								if ( chosenPath != null && path.length() >= chosenPath.length()
								        && path.compareTo ( chosenPath ) > 0 ) {
									// return nextPermutation(true);
									if ( !permutator.hasNext() ) {
										// digest chosen path and update namer
										md.update ( chosenPath.getBytes ( "UTF-8" ) );
										pathNamer = chosenNamer;
										// hash the nextGroup
										breakOut = true;
									}
									break;
								}
								// do next recursion
							}

							// if we should stop processing this group
							if ( breakOut )
								break;
						}
					}
				}

				// get adjacent bnode
				const Map<String, Object> quad = ( Map<String, Object> ) quads.get ( hpi );
				String bnode = getAdjacentBlankNodeName ( ( Map<String, Object> ) quad.get ( "subject" ),
				               id );
				String direction = null;
				if ( bnode != null ) {
					// normal property
					direction = "p";
				} else {
					bnode = getAdjacentBlankNodeName ( ( Map<String, Object> ) quad.get ( "object" ), id );
					if ( bnode != null ) {
						// reverse property
						direction = "r";
					}
				}

				if ( bnode != null ) {
					// get bnode name (try canonical, path, then hash)
					String name;
					if ( namer.isNamed ( bnode ) )
						name = namer.getName ( bnode ); else if ( pathNamer.isNamed ( bnode ) )
						name = pathNamer.getName ( bnode ); else
						name = hashQuads ( bnode, bnodes, namer );

					// hash direction, property, end bnode name/hash
					const MessageDigest md1 = MessageDigest.getInstance ( "SHA-1" );
					// String toHash = direction + (String) ((Map<String,
					// Object>) quad.get("predicate")).get("value") + name;
					md1.update ( direction.getBytes ( "UTF-8" ) );
					md1.update ( ( ( String ) ( ( Map<String, Object> ) quad.get ( "predicate" ) ).get ( "value" ) )
					             .getBytes ( "UTF-8" ) );
					md1.update ( name.getBytes ( "UTF-8" ) );
					const String groupHash = encodeHex ( md1.digest() );
					if ( groups.containsKey ( groupHash ) )
						groups.get ( groupHash ).add ( bnode ); else {
						const List<String> tmp = new ArrayList<String>();
						tmp.add ( bnode );
						groups.put ( groupHash, tmp );
					}
				}
			}
		} catch ( const NoSuchAlgorithmException e ) {
			// TODO: i don't expect that SHA-1 is even NOT going to be
			// available?
			// look into this further
			throw new RuntimeException ( e );
		} catch ( const UnsupportedEncodingException e ) {
			// TODO: i don't expect that UTF-8 is ever not going to be available
			// either
			throw new RuntimeException ( e );
		}
	}

	/**
	    Hashes all of the quads about a blank node.

	    @param id
	              the ID of the bnode to hash quads for.
	    @param bnodes
	              the mapping of bnodes to quads.
	    @param namer
	              the canonical bnode namer.

	    @return the new hash.
	*/
private:
	static String hashQuads ( String id, Map<String, Object> bnodes, UniqueNamer namer ) {
		// return cached hash
		if ( ( ( Map<String, Object> ) bnodes.get ( id ) ).containsKey ( "hash" ) )
			return ( String ) ( ( Map<String, Object> ) bnodes.get ( id ) ).get ( "hash" );

		// serialize all of bnode's quads
		const List<Map<String, Object>> quads = ( List<Map<String, Object>> ) ( ( Map<String, Object> ) bnodes
		                                        .get ( id ) ).get ( "quads" );
		const List<String> nquads = new ArrayList<String>();
		for ( int i = 0; i < quads.size(); ++i ) {
			nquads.add ( toNQuad ( ( RDFDataset.Quad ) quads.get ( i ),
			                       quads.get ( i ).get ( "name" ) != null ? ( String ) ( ( Map<String, Object> ) quads.get ( i )
			                               .get ( "name" ) ).get ( "value" ) : null, id ) );
		}
		// sort serialized quads
		Collections.sort ( nquads );
		// return hashed quads
		const String hash = sha1hash ( nquads );
		( ( Map<String, Object> ) bnodes.get ( id ) ).put ( "hash", hash );
		return hash;
	}

	/**
	    A helper class to sha1 hash all the strings in a collection

	    @param nquads
	    @return
	*/
	/*	 static String sha1hash ( Collection<String> nquads ) {
			try {
				// create SHA-1 digest
				const MessageDigest md = MessageDigest.getInstance ( "SHA-1" );
				for ( const String nquad : nquads )
					md.update ( nquad.getBytes ( "UTF-8" ) );
				return encodeHex ( md.digest() );
			} catch ( const NoSuchAlgorithmException e ) {
				throw new RuntimeException ( e );
			} catch ( const UnsupportedEncodingException e ) {
				throw new RuntimeException ( e );
			}
		}
	*/
	// TODO: this is something to optimize
	static String encodeHex ( const byte[] data ) {
		String rval = "";
		for ( const byte b : data )
			rval += String.format ( "%02x", b );
		return rval;
	}

	/**
	    A helper function that gets the blank node name from an RDF quad node
	    (subject or object). If the node is a blank node and its value does not
	    match the given blank node ID, it will be returned.

	    @param node
	              the RDF quad node.
	    @param id
	              the ID of the blank node to look next to.

	    @return the adjacent blank node name or null if none was found.
	*/
	static String getAdjacentBlankNodeName ( Map<String, Object> node, String id ) {
		return "blank node".equals ( node.get ( "type" ) )
		       && ( !node.containsKey ( "value" ) || !Obj.equals ( node.get ( "value" ), id ) ) ? ( String ) node
		       .get ( "value" ) : null;
	}

	class Permutator {
		const List<String> list;
		boolean done;
		const Map<String, Boolean> left;

	public:
		Permutator ( List<String> list_ ) :
			list ( list_ ) { // = ( List<String> ) JsonLdUtils.clone ( list );
			std::sort ( list.begin(), list.end() );
			done = false;
			//			left = new LinkedHashMap<String, Boolean>();
			for ( const String i : list ) left.put ( i, true );
		}

		/**
		    Returns true if there is another permutation.

		    @return true if there is another permutation, false if not.
		*/
		boolean hasNext() {
			return !done;
		}

		/**
		    Gets the next permutation. Call hasNext() to ensure there is another
		    one first.

		    @return the next permutation.
		*/
		List<String> next() {
			const List<String> rval = list;// = ( List<String> ) JsonLdUtils.clone ( this.list );

			// Calculate the next permutation using Steinhaus-Johnson-Trotter
			// permutation algoritm

			// get largest mobile element k
			// (mobile: element is grater than the one it is looking at)
			String k = null;
			int pos = 0;
			const int length = list.size();
			for ( int i = 0; i < length; ++i ) {
				const String element = list.at ( i );
				const Boolean left = left.at ( element );
				if ( ( k == null || element.compareTo ( k ) > 0 )
				        && ( ( left && i > 0 && element.compareTo ( list.at ( i - 1 ) ) > 0 ) || ( !left
				                && i < ( length - 1 ) && element.compareTo ( list.at ( i + 1 ) ) > 0 ) ) ) {
					k = element;
					pos = i;
				}
			}

			// no more permutations
			if ( k == null ) done = true;
			else {
				// swap k and the element it is looking at
				const int swap = left.get ( k ) ? pos - 1 : pos + 1;
				list[pos] = list.get ( swap ) ;
				list.[swap] = k ;

				// reverse the direction of all element larger than k
				for ( int i = 0; i < length; i++ ) {
					if ( list.at ( i ).compareTo ( k ) > 0 )
						left[list.get ( i )] = !left.get ( list.get ( i ) ) ;
				}
			}

			return rval;
		}

	};

};
