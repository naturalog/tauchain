class jsonld_api {
public:
	jsonld_options opts;
	pobj value = 0;
	pcontext context = 0;

	jsonld_api ( pobj input, jsonld_options opts ) : jsonld_api ( opts )  {
		initialize ( input, 0 );
	}
	jsonld_api ( pobj input, pobj context, jsonld_options opts ) : jsonld_api ( opts )  {
		initialize ( input, 0 );
	}
	jsonld_api ( jsonld_options opts_ = jsonld_options ( "" ) ) : opts ( opts_ ) {}
private:
	void initialize ( pobj input, pobj context_ ) {
		if ( input->LIST() || input->MAP() ) value = input->clone();
		context = make_shared<context_t>();
		if ( context ) context = make_shared<context_t> ( context->parse ( context_ ) );
	}
public:
	// http://json-ld.org/spec/latest/json-ld-api/#compaction-algorithm
	pobj compact ( pcontext activeCtx, string activeProperty, pobj element, bool compactArrays ) {
		if ( element->LIST() ) {
			polist result = make_shared<olist>();
			for ( pobj item : *element->LIST() ) {
				pobj compactedItem = compact ( activeCtx, activeProperty, item, compactArrays );
				if ( compactedItem ) result->push_back ( compactedItem );
			}
			return ( compactArrays && result->size() == 1 && !activeCtx->getContainer ( activeProperty ) ) ? result->at ( 0 ) : make_shared<olist_obj> ( result );
		}
		if ( !element->MAP() ) return element;

		psomap elem = element->MAP();
		if ( has ( elem, "@value" ) || has ( elem, "@id" ) )
			// TODO: spec tells to pass also inverse to compactValue
			if ( pobj compacted_val = activeCtx->compactValue ( activeProperty, make_shared<somap_obj> ( elem ) ) )
				if ( ! ( compacted_val->MAP() || compacted_val->LIST() ) )
					return compacted_val;

		bool insideReverse = activeProperty == "@reverse";
		psomap result = make_shared<somap>();
		for ( auto x : *elem ) { // 7
			string exp_prop = x.first;
			pobj exp_val = x.second;
			if ( is ( exp_prop, { "@id"s, "@type"s} ) ) {
				pobj compacted_val;
				// TODO: spec tells to pass also inverse to compactIri
				if ( exp_val->STR() ) compacted_val = activeCtx->compactIri ( exp_val->STR(), exp_prop == "@type" );
				else {
					vector<string> types;
					for ( auto expandedType : *exp_val->LIST() ) types.push_back ( *activeCtx->compactIri ( expandedType->STR(), true ) );
					if ( types.size() == 1 ) compacted_val = make_shared<string_obj> ( types[0] );
					else compacted_val = vec2vec ( types );
				}
				pstring alias = activeCtx->compactIri ( exp_prop, true );
				result->at ( *alias ) = compacted_val;
				continue;
			}
			if ( exp_prop == "@reverse" ) {
				psomap compacted_val = compact ( activeCtx, "@reverse", exp_val, compactArrays )->MAP();
				// Must create a new set to avoid modifying the set we are iterating over
				for ( auto y : somap ( *compacted_val ) ) {
					string property = y.first;
					pobj value = y.second;
					if ( activeCtx->isReverseProperty ( property ) ) {
						if ( ( *activeCtx->getContainer ( property ) == "@set" || !compactArrays ) && !value->LIST() )
							value = make_shared<olist_obj> ( 1, value );
						if ( !has ( result, property ) ) result->at ( property ) = value;
						else {
							make_list_if_not ( result->at ( property ) );
							add_all ( result->at ( property )->LIST(), value );
						}
						compacted_val->erase ( property );
					}
				}
				if ( compacted_val->size() ) result->at ( *activeCtx->compactIri ( "@reverse", true ) ) = compacted_val;
				continue;
			}
			if ( exp_prop == "@index" && *activeCtx->getContainer ( activeProperty ) == "@index" ) continue;
			if ( is ( exp_prop, {"@index"s, "@value"s, "@language"s} ) ) {
				result->at ( *activeCtx->compactIri ( exp_prop, true ) ) = exp_val;
				continue;
			}
			if ( !exp_val->LIST()->size() ) {
				string itemActiveProperty = *activeCtx->compactIri ( exp_prop, exp_val, true, insideReverse );
				auto it = result->find ( itemActiveProperty );
				if ( it == result->end() ) result->at ( itemActiveProperty ) = make_shared<olist_obj>();
				else make_list_if_not ( it->second );

			}
			for ( pobj exp_item : *exp_val->LIST() ) {
				string itemActiveProperty = *activeCtx->compactIri ( exp_prop, exp_item, true, insideReverse );
				string container = *activeCtx->getContainer ( itemActiveProperty );
				bool isList = has ( exp_item->MAP(), "@list" );
				pobj list = isList ? exp_item->MAP()->at ( "@list" ) : 0;
				pobj compactedItem = compact ( activeCtx, itemActiveProperty, isList ? list : exp_item, compactArrays );
				if ( isList ) {
					make_list_if_not ( compactedItem );
					if ( container != "@list" ) {
						psomap wrapper = make_shared<somap>();
						wrapper->at ( *activeCtx->compactIri ( "@list", true ) ) = compactedItem;
						compactedItem = wrapper;
						if ( has ( exp_item->MAP(), "@index" ) ) {
							compactedItem->MAP()->at (
							    // TODO: SPEC: no mention of vocab =
							    // true
							    *activeCtx->compactIri ( "@index", true ) ) = exp_item->MAP()->at ( "@index" ) ;
						}
					} else if ( has ( result, itemActiveProperty ) ) throw COMPACTION_TO_LIST_OF_LISTS + "\t" + "There cannot be two list objects associated with an active property that has a container mapping";
				}
				if ( is ( container, {"@language"s, "@index"s} ) ) {
					psomap_obj mapObject;
					if ( has ( result, itemActiveProperty ) ) mapObject = result->at ( itemActiveProperty )->MAP();
					else result->at ( itemActiveProperty ) = mapObject = make_shared<somap_obj>();
					if ( container == "@language" && has ( compactedItem->MAP(), "@value" ) )
						compactedItem = compactedItem->MAP()->at ( "@value" );
					string mapKey = *exp_item->MAP() ->at ( container )->STR();
					if ( !has ( mapObject, mapKey ) ) mapObject->MAP()->at ( mapKey ) = compactedItem;
					else {
						make_list_if_not ( mapObject->MAP()->at ( mapKey ) );
						mapObject->MAP()->at ( mapKey )->LIST()->push_back ( compactedItem );
					}
				}
				else {
					bool check = ( !compactArrays || is ( container, {"@set"s, "@list"} ) || is ( exp_prop, {"@list"s, "@graph"s} ) ) && ( !compactedItem->LIST() );
					if ( check ) compactedItem = make_shared<olist_obj> ( olist ( 1, compactedItem ) );
					if ( !has ( result, itemActiveProperty ) )  result->at ( itemActiveProperty ) = compactedItem;
					else {
						make_list_if_not ( result->at ( itemActiveProperty ) );
						add_all ( result->at ( itemActiveProperty )->LIST(), compactedItem );
					}
				}
			}
		}
		return make_shared<somap_obj> ( result );
	}

	pobj compact ( pcontext activeCtx, string activeProperty, pobj element ) {
		return compact ( activeCtx, activeProperty, element, true );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm
	pobj expand ( pcontext& activeCtx, const string& activeProperty, pobj& element ) {
		if ( !element )  return 0;
		if ( element->LIST() ) {
			polist_obj result = make_shared<olist_obj>();
			for ( pobj item : *element->LIST() ) {
				pobj v = expand ( activeCtx, activeProperty, item );
				if ( ( activeProperty == "@list" || *activeCtx->getContainer ( activeProperty ) == "@list" )
				        && ( v->LIST() || ( v->MAP() && has ( v->MAP(), "@list" ) ) ) )
					throw LIST_OF_LISTS + "\t"s + "lists of lists are not permitted.";
				if ( v ) add_all ( result->LIST(), v );

			}
			return result;
		} else if ( element->MAP() ) {
			psomap elem = element->MAP();
			if ( has ( elem, "@context" ) ) activeCtx = activeCtx->parse ( elem->at ( "@context" ) );
			psomap result = make_shared<somap>();
			for ( auto x : *elem ) {
				string key = x.first;
				pobj value = x.second;
				if ( key == "@context" ) continue;
				pstring exp_prop = activeCtx->expandIri ( key, false, true, 0, 0 );
				pobj exp_val = 0;
				if ( !exp_prop || ( exp_prop->find ( ":" ) == string::npos && !keyword ( *exp_prop ) ) ) continue;
				if ( keyword ( *exp_prop ) ) {
					if ( activeProperty == "@reverse" ) throw INVALID_REVERSE_PROPERTY_MAP + "\t"s + "a keyword cannot be used as a @reverse propery";
					if ( has ( result, exp_prop ) ) throw COLLIDING_KEYWORDS + "\t" + *exp_prop + " already exists in result";
					if ( *exp_prop == "@id" ) {
						if ( !value->STR() ) throw INVALID_ID_VALUE + "\t" + "value of @id must be a string";
						exp_val = activeCtx->expandIri ( value->STR(), true, false, 0, 0 );
					} else if ( *exp_prop == "@type" ) {
						if ( value->LIST() ) {
							exp_val = make_shared<olist_obj>();
							for ( pobj v :  *value->LIST() ) {
								if ( !v->STR() ) throw INVALID_TYPE_VALUE + "\t" + "@type value must be a string or array of strings" ;
								exp_val->LIST()->push_back ( make_shared<string_obj> ( activeCtx->expandIri ( v->STR(), true, true, 0, 0 ) ) );
							}
						} else if ( value->STR() ) exp_val = activeCtx->expandIri ( value->STR(), true, true, 0, 0 );
						else if ( value->MAP() ) {
							if ( value->MAP()->size() ) throw INVALID_TYPE_VALUE + "\t" + "@type value must be a an empty object for framing";
							exp_val = value;
						} else
							throw INVALID_TYPE_VALUE + "\t" + "@type value must be a string or array of strings";
					} else if ( *exp_prop == "@graph" ) exp_val = expand ( activeCtx, "@graph", value );
					else if ( *exp_prop == "@value" ) {
						if ( value && ( value->MAP() || value->LIST() ) ) throw INVALID_VALUE_OBJECT_VALUE + "\t"s + "value of " + *exp_prop + " must be a scalar or null";
						if ( ! ( exp_val = value ) ) {
							result->at ( "@value" ) = 0;
							continue;
						}
					} else if ( *exp_prop == "@language" ) {
						if ( !value->STR() ) throw INVALID_LANGUAGE_TAGGED_STRING + "\t" + "Value of "s + *exp_prop + " must be a string";
						exp_val = make_shared<string_obj> ( lower ( *value->STR() ) );
					} else if ( *exp_prop == "@index" ) {
						if ( !value->STR() ) throw INVALID_INDEX_VALUE + "\t" + "Value of "s + *exp_prop + " must be a string";
						exp_val = value;
					} else if ( *exp_prop == "@list" ) {
						if ( activeProperty == "@graph" ) continue;
						exp_val = expand ( activeCtx, activeProperty, value );
						if ( !exp_val->LIST() ) exp_val = make_shared<olist_obj> ( olist ( 1, exp_val ) );
						for ( auto o : *exp_val->LIST() ) if ( o->MAP() && has ( o->MAP(), "@list" ) ) throw LIST_OF_LISTS + "\t" + "A list may not contain another list";
					} else if ( *exp_prop == "@set" )
						exp_val = expand ( activeCtx, activeProperty, value );
					else if ( *exp_prop == "@reverse" ) {
						if ( !value->MAP() ) throw INVALID_REVERSE_VALUE + "\t" + "@reverse value must be an object";
						exp_val = expand ( activeCtx, "@reverse", value );
						if ( has ( exp_val->MAP(), "@reverse" ) ) {
							psomap reverse = exp_val->MAP()->at ( "@reverse" )->MAP();
							for ( auto z : *reverse ) {
								string property = z.first;
								pobj item = z.second;
								if ( !has ( result, property ) ) result->at ( property ) = make_shared<olist_obj>();
								add_all ( result->at ( property )->LIST(), item );
							}
						}
						if ( exp_val->MAP()->size() > ( has ( exp_val->MAP(), "@reverse" ) ? 1 : 0 ) ) {
							if ( !has ( result, "@reverse" ) )  result->at ( "@reverse" ) = make_shared<somap_obj>();
							psomap reverseMap = result->at ( "@reverse" )->MAP();
							for ( auto t : *exp_val->MAP() ) {
								string property = t.first;
								if ( property == "@reverse" ) continue;
								polist items = exp_val->MAP()->at ( property )->LIST();
								for ( pobj item : *items ) {
									if ( has ( item->MAP(), "@value" ) || has ( item->MAP(), "@list" ) ) throw INVALID_REVERSE_PROPERTY_VALUE;
									if ( !has ( reverseMap, property ) ) reverseMap->at ( property ) = make_shared<olist_obj>();
									reverseMap->at ( property )->LIST()->push_back ( item );
								}
							}
						}
						continue;
					} else if ( is ( exp_prop, {"@explicit"s, "@default"s, "@embed"s, "@embedChildren"s, "@omitDefault"s} ) )
						exp_val = expand ( activeCtx, *exp_prop, value );
					if ( exp_val ) result->at ( *exp_prop ) = exp_val;
					continue;
				} else if ( *activeCtx->getContainer ( key ) == "@language" && value->MAP() ) {
					exp_val = make_shared<olist_obj>();
					for ( auto yy : *value->MAP() ) {
						string language = yy.first;
						pobj languageValue = yy.second;
						make_list_if_not ( languageValue );
						for ( pobj item : *languageValue->LIST() ) {
							if ( ! item->STR() ) throw INVALID_LANGUAGE_MAP_VALUE; // + "\t" + "Expected " + item.toString() + " to be a string");
							somap tmp;
							tmp["@value"] = item;
							tmp["@language"] = make_shared<string_obj> ( lower ( language ) );
							exp_val->LIST( )->push_back ( make_shared<somap_obj> ( tmp ) );
						}
					}
				} else if ( *activeCtx->getContainer ( key ) == "@index" && value->MAP() ) {
					exp_val = make_shared<olist_obj>();
					List<String> indexKeys = new ArrayList<String> ( ( ( Map<String, Object> ) value ).keySet() );
					Collections.sort ( indexKeys );
					for ( final String index : indexKeys ) {
						Object indexValue = ( ( Map<String, Object> ) value ).get ( index );
						if ( ! ( indexValue->LIST() ) ) {
							final Object tmp = indexValue;
							indexValue = new ArrayList<Object>();
							( ( List<Object> ) indexValue ).add ( tmp );
						}
						indexValue = expand ( activeCtx, key, indexValue );
						for ( final Map<String, Object> item : ( List<Map<String, Object>> ) indexValue ) {
							if ( !has ( item, "@index" ) ) item->at ( "@index" ) = index;
							exp_val->LIST()->push_back ( item );
						}
					}
				} else exp_val = expand ( activeCtx, key, value );
				if ( !exp_val ) continue;
				if ( activeCtx->getContainer ( key ) == "@list" ) {
					if ( !has ( exp_val->MAP(), "@list" ) ) {
						Object tmp = exp_val;
						if ( !tmp->LIST() ) {
							tmp = new ArrayList<Object>();
							( ( List<Object> ) tmp ).add ( exp_val );
						}
						exp_val = newMap();
						( ( Map<String, Object> ) exp_val ).put ( "@list", tmp );
					}
				}
				if ( activeCtx.isReverseProperty ( key ) ) {
					if ( !result.containsKey ( "@reverse" ) ) result.put ( "@reverse", newMap() );
					final Map<String, Object> reverseMap = ( Map<String, Object> ) result .get ( "@reverse" );
					if ( !exp_val->LIST() ) {
						final Object tmp = exp_val;
						exp_val = new ArrayList<Object>();
						( ( List<Object> ) exp_val ).add ( tmp );
					}
					for ( final Object item : ( List<Object> ) exp_val ) {
						if ( has ( item->MAP(), "@value" ) && has ( item->MAP(), "@list" ) ) throw INVALID_REVERSE_PROPERTY_VALUE;
						if ( !has ( reverseMap, exp_prop ) ) reverseMap->at ( exp_prop ) = make_shared<olist_obj>();
						add_all ( reverseMap->at ( exp_prop )->LIST(), item );
					}
				} else {
					if ( !has ( result, exp_prop ) ) result->at ( exp_prop ) = make_shared<olist_obj>();
					add_all ( result->at ( exp_prop )->LIST(), exp_val );
				}
			}
			if ( has ( result, "@value" ) ) {
				final Set<String> keySet = new HashSet ( result.keySet() );
				keySet.remove ( "@value" );
				keySet.remove ( "@index" );
				final boolean langremoved = keySet.remove ( "@language" );
				final boolean typeremoved = keySet.remove ( "@type" );
				if ( ( langremoved && typeremoved ) || !keySet.isEmpty() ) throw INVALID_VALUE_OBJECT + "\t" + "value object has unknown keys";
				pobj rval = result->at ( "@value" );
				if ( !rval ) return 0;
				if ( ! ( rval->STR() ) && has ( result, "@language" ) ) throw INVALID_LANGUAGE_TAGGED_VALUE + "\t" + "when @language is used, @value must be a string";
				else if ( has ( result, "@type" ) ) {
					if ( ! ( result->at ( "@type" )->STR() ) || startsWith ( result->at ( "@type" )->STR(), "_:" ) || result->at ( "@type" )->STR()->find ( ":" ) == string::npos )
						throw INVALID_TYPED_VALUE + "\t" + "value of @type must be an IRI";
				}
			} else if ( has ( result, "@type" ) ) {
				pobj rtype = result->at ( "@type" );
				if ( !rtype->LIST() ) result.put ( "@type", make_shared<olist_obj> ( olist ( 1, rtype ) ) );
			} else if ( has ( result, "@set" ) || has ( result, "@list" ) ) {
				if ( result.size() > ( result.containsKey ( "@index" ) ? 2 : 1 ) )
					throw INVALID_SET_OR_LIST_OBJECT + "\t" + "@set or @list may only contain @index";
				if ( has ( result, "@set" ) )
					return result.get ( "@set" );
			}
			if ( ( has ( result, "@language" ) && result.size() == 1 ) ||  ( !activeProperty || *activeProperty == "@graph" ) && result && ( ( !result.size() || has ( result, "@value" ) || has ( result , "@list" ) ) ) ||
			        ( has ( result, "@id" ) && result.size() == 1 ) ) result = 0;
			return result;
		} else {
			if ( !activeProperty  || *activeProperty == "@graph" ) return 0;
			return activeCtx->expandValue ( activeProperty, element );
		}
	}
};
#ifdef AAA
/**
    Expansion Algorithm

    http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm

    @param activeCtx
              The Active Context
    @param element
              The current element
    @return The expanded JSON-LD object.
    @throws JsonLdError
               If there was an error during expansion.
*/
public Object expand ( Context activeCtx, Object element ) throws JsonLdError {
	return expand ( activeCtx, null, element );
}

/***
    _____ _ _ _ _ _ _ _ _ | ___| | __ _| |_| |_ ___ _ __ / \ | | __ _ ___ _
    __(_) |_| |__ _ __ ___ | |_ | |/ _` | __| __/ _ \ '_ \ / _ \ | |/ _` |/ _
    \| '__| | __| '_ \| '_ ` _ \ | _| | | (_| | |_| || __/ | | | / ___ \| |
    (_| | (_) | | | | |_| | | | | | | | | |_| |_|\__,_|\__|\__\___|_| |_| /_/
    \_\_|\__, |\___/|_| |_|\__|_| |_|_| |_| |_| |___/
*/

void generateNodeMap ( Object element, Map<String, Object> nodeMap ) throws JsonLdError {
	generateNodeMap ( element, nodeMap, "@default", null, null, null );
}

void generateNodeMap ( Object element, Map<String, Object> nodeMap, String activeGraph )
throws JsonLdError {
	generateNodeMap ( element, nodeMap, activeGraph, null, null, null );
}

void generateNodeMap ( Object element, Map<String, Object> nodeMap, String activeGraph,
                       Object activeSubject, String activeProperty, Map<String, Object> list )
throws JsonLdError {
	// 1)
	if ( element->LIST() ) {
		// 1.1)
		for ( final Object item : ( List<Object> ) element )
			generateNodeMap ( item, nodeMap, activeGraph, activeSubject, activeProperty, list );
		return;
	}

	// for convenience
	final Map<String, Object> elem = ( Map<String, Object> ) element;

	// 2)
	if ( !nodeMap.containsKey ( activeGraph ) )
		nodeMap.put ( activeGraph, newMap() );
	final Map<String, Object> graph = ( Map<String, Object> ) nodeMap.get ( activeGraph );
	Map<String, Object> node = ( Map<String, Object> ) ( activeSubject == null ? null : graph
	.get ( activeSubject ) );

	// 3)
	if ( elem.containsKey ( "@type" ) ) {
		// 3.1)
		List<String> oldTypes;
		final List<String> newTypes = new ArrayList<String>();
		if ( elem.get ( "@type" )->LIST() )
			oldTypes = ( List<String> ) elem.get ( "@type" ); else {
			oldTypes = new ArrayList<String>();
			oldTypes.add ( ( String ) elem.get ( "@type" ) );
		}
		for ( final String item : oldTypes ) {
			if ( item.startsWith ( "_:" ) )
				newTypes.add ( generateBlankNodeIdentifier ( item ) ); else
				newTypes.add ( item );
		}
		if ( elem.get ( "@type" )->LIST() )
			elem.put ( "@type", newTypes ); else
			elem.put ( "@type", newTypes.get ( 0 ) );
	}

	// 4)
	if ( elem.containsKey ( "@value" ) ) {
		// 4.1)
		if ( list == null )
			JsonLdUtils.mergeValue ( node, activeProperty, elem );
		// 4.2)
		else
			JsonLdUtils.mergeValue ( list, "@list", elem );
	}

	// 5)
	else if ( elem.containsKey ( "@list" ) ) {
		// 5.1)
		final Map<String, Object> result = newMap ( "@list", new ArrayList<Object>() );
		// 5.2)
		// for (final Object item : (List<Object>) elem.get("@list")) {
		// generateNodeMap(item, nodeMap, activeGraph, activeSubject,
		// activeProperty, result);
		// }
		generateNodeMap ( elem.get ( "@list" ), nodeMap, activeGraph, activeSubject, activeProperty,
		                  result );
		// 5.3)
		JsonLdUtils.mergeValue ( node, activeProperty, result );
	}

	// 6)
	else {
		// 6.1)
		String id = ( String ) elem.remove ( "@id" );
		if ( id != null ) {
			if ( id.startsWith ( "_:" ) )
				id = generateBlankNodeIdentifier ( id );
		}
		// 6.2)
		else
			id = generateBlankNodeIdentifier ( null );
		// 6.3)
		if ( !graph.containsKey ( id ) ) {
			final Map<String, Object> tmp = newMap ( "@id", id );
			graph.put ( id, tmp );
		}
		// 6.4) TODO: SPEC this line is asked for by the spec, but it breaks
		// various tests
		// node = (Map<String, Object>) graph.get(id);
		// 6.5)
		if ( activeSubject->MAP() ) {
			// 6.5.1)
			JsonLdUtils.mergeValue ( ( Map<String, Object> ) graph.get ( id ), activeProperty,
			activeSubject );
		}
		// 6.6)
		else if ( activeProperty != null ) {
			final Map<String, Object> reference = newMap ( "@id", id );
			// 6.6.2)
			if ( list == null ) {
				// 6.6.2.1+2)
				JsonLdUtils.mergeValue ( node, activeProperty, reference );
			}
			// 6.6.3) TODO: SPEC says to add ELEMENT to @list member, should
			// be REFERENCE
			else
				JsonLdUtils.mergeValue ( list, "@list", reference );
		}
		// TODO: SPEC this is removed in the spec now, but it's still needed
		// (see 6.4)
		node = ( Map<String, Object> ) graph.get ( id );
		// 6.7)
		if ( elem.containsKey ( "@type" ) ) {
			for ( final Object type : ( List<Object> ) elem.remove ( "@type" ) )
				JsonLdUtils.mergeValue ( node, "@type", type );
		}
		// 6.8)
		if ( elem.containsKey ( "@index" ) ) {
			final Object elemIndex = elem.remove ( "@index" );
			if ( node.containsKey ( "@index" ) ) {
				if ( !JsonLdUtils.deepCompare ( node.get ( "@index" ), elemIndex ) )
					throw new JsonLdError ( Error.CONFLICTING_INDEXES );
			} else
				node.put ( "@index", elemIndex );
		}
		// 6.9)
		if ( elem.containsKey ( "@reverse" ) ) {
			// 6.9.1)
			final Map<String, Object> referencedNode = newMap ( "@id", id );
			// 6.9.2+6.9.4)
			final Map<String, Object> reverseMap = ( Map<String, Object> ) elem
			                                       .remove ( "@reverse" );
			// 6.9.3)
			for ( final String property : reverseMap.keySet() ) {
				final List<Object> values = ( List<Object> ) reverseMap.get ( property );
				// 6.9.3.1)
				for ( final Object value : values ) {
					// 6.9.3.1.1)
					generateNodeMap ( value, nodeMap, activeGraph, referencedNode, property, null );
				}
			}
		}
		// 6.10)
		if ( elem.containsKey ( "@graph" ) )
			generateNodeMap ( elem.remove ( "@graph" ), nodeMap, id, null, null, null );
		// 6.11)
		final List<String> keys = new ArrayList<String> ( elem.keySet() );
		Collections.sort ( keys );
		for ( String property : keys ) {
			final Object value = elem.get ( property );
			// 6.11.1)
			if ( property.startsWith ( "_:" ) )
				property = generateBlankNodeIdentifier ( property );
			// 6.11.2)
			if ( !node.containsKey ( property ) )
				node.put ( property, new ArrayList<Object>() );
			// 6.11.3)
			generateNodeMap ( value, nodeMap, activeGraph, id, property, null );
		}
	}
}

/**
    Blank Node identifier map specified in:

    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier
*/
private final Map<String, String> blankNodeIdentifierMap = new LinkedHashMap<String, String>();

/**
    Counter specified in:

    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier
*/
private int blankNodeCounter = 0;

/**
    Generates a blank node identifier for the given key using the algorithm
    specified in:

    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier

    @param id
              The id, or null to generate a fresh, unused, blank node
              identifier.
    @return A blank node identifier based on id if it was not null, or a
           fresh, unused, blank node identifier if it was null.
*/
String generateBlankNodeIdentifier ( String id ) {
	if ( id != null && blankNodeIdentifierMap.containsKey ( id ) )
		return blankNodeIdentifierMap.get ( id );
	final String bnid = "_:b" + blankNodeCounter++;
	if ( id != null )
		blankNodeIdentifierMap.put ( id, bnid );
	return bnid;
}

/**
    Generates a fresh, unused, blank node identifier using the algorithm
    specified in:

    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier

    @return A fresh, unused, blank node identifier.
*/
String generateBlankNodeIdentifier() {
	return generateBlankNodeIdentifier ( null );
}

/***
    _____ _ _ _ _ _ _ | ___| __ __ _ _ __ ___ (_)_ __ __ _ / \ | | __ _ ___ _
    __(_) |_| |__ _ __ ___ | |_ | '__/ _` | '_ ` _ \| | '_ \ / _` | / _ \ |
    |/ _` |/ _ \| '__| | __| '_ \| '_ ` _ \ | _|| | | (_| | | | | | | | | | |
    (_| | / ___ \| | (_| | (_) | | | | |_| | | | | | | | | |_| |_| \__,_|_|
    |_| |_|_|_| |_|\__, | /_/ \_\_|\__, |\___/|_| |_|\__|_| |_|_| |_| |_|
    |___/ |___/
*/

private class FramingContext {
	public boolean embed;
	public boolean explicit;
	public boolean omitDefault;

	public FramingContext() {
		embed = true;
		explicit = false;
		omitDefault = false;
		embeds = null;
	}

	public FramingContext ( jsonld_options opts ) {
		this();
		if ( opts.getEmbed() != null )
			this.embed = opts.getEmbed();
		if ( opts.getExplicit() != null )
			this.explicit = opts.getExplicit();
		if ( opts.getOmitDefault() != null )
			this.omitDefault = opts.getOmitDefault();
	}

	public Map<String, EmbedNode> embeds = null;
}

private class EmbedNode {
	public Object parent = null;
	public String property = null;
}

private Map<String, Object> nodeMap;

/**
    Performs JSON-LD <a
    href="http://json-ld.org/spec/latest/json-ld-framing/">framing</a>.

    @param input
              the expanded JSON-LD to frame.
    @param frame
              the expanded JSON-LD frame to use.
    @return the framed output.
    @throws JsonLdError
               If the framing was not successful.
*/
public List<Object> frame ( Object input, List<Object> frame ) throws JsonLdError {
	// create framing state
	final FramingContext state = new FramingContext ( this.opts );

	// use tree map so keys are sotred by default
	final Map<String, Object> nodes = new TreeMap<String, Object>();
	generateNodeMap ( input, nodes );
	this.nodeMap = ( Map<String, Object> ) nodes.get ( "@default" );

	final List<Object> framed = new ArrayList<Object>();
	// NOTE: frame validation is done by the function not allowing anything
	// other than list to me passed
	frame ( state,
	this.nodeMap,
	( frame != null && frame.size() > 0 ? ( Map<String, Object> ) frame.get ( 0 ) : newMap() ),
	framed, null );

	return framed;
}

/**
    Frames subjects according to the given frame.

    @param state
              the current framing state.
    @param subjects
              the subjects to filter.
    @param frame
              the frame.
    @param parent
              the parent subject or top-level array.
    @param property
              the parent property, initialized to null.
    @throws JsonLdError
               If there was an error during framing.
*/
private void frame ( FramingContext state, Map<String, Object> nodes, Map<String, Object> frame,
                     Object parent, String property ) throws JsonLdError {

	// filter out subjects that match the frame
	final Map<String, Object> matches = filterNodes ( state, nodes, frame );

	// get flags for current frame
	Boolean embedOn = getFrameFlag ( frame, "@embed", state.embed );
	final Boolean explicicOn = getFrameFlag ( frame, "@explicit", state.explicit );

	// add matches to output
	final List<String> ids = new ArrayList<String> ( matches.keySet() );
	Collections.sort ( ids );
	for ( final String id : ids ) {
		if ( property == null )
			state.embeds = new LinkedHashMap<String, EmbedNode>();

		// start output
		final Map<String, Object> output = newMap();
		output.put ( "@id", id );

		// prepare embed meta info
		final EmbedNode embeddedNode = new EmbedNode();
		embeddedNode.parent = parent;
		embeddedNode.property = property;

		// if embed is on and there is an existing embed
		if ( embedOn && state.embeds.containsKey ( id ) ) {
			final EmbedNode existing = state.embeds.get ( id );
			embedOn = false;

			if ( existing.parent->LIST() ) {
				for ( final Object p : ( List<Object> ) existing.parent ) {
					if ( JsonLdUtils.compareValues ( output, p ) ) {
						embedOn = true;
						break;
					}
				}
			}
			// existing embed's parent is an object
			else {
				if ( ( ( Map<String, Object> ) existing.parent ).containsKey ( existing.property ) ) {
					for ( final Object v : ( List<Object> ) ( ( Map<String, Object> ) existing.parent )
					        .get ( existing.property ) ) {
						if ( v->MAP()
						        && Obj.equals ( id, ( ( Map<String, Object> ) v ).get ( "@id" ) ) ) {
							embedOn = true;
							break;
						}
					}
				}
			}

			// existing embed has already been added, so allow an overwrite
			if ( embedOn )
				removeEmbed ( state, id );
		}

		// not embedding, add output without any other properties
		if ( !embedOn )
			addFrameOutput ( state, parent, property, output ); else {
			// add embed meta info
			state.embeds.put ( id, embeddedNode );

			// iterate over subject properties
			final Map<String, Object> element = ( Map<String, Object> ) matches.get ( id );
			List<String> props = new ArrayList<String> ( element.keySet() );
			Collections.sort ( props );
			for ( final String prop : props ) {

				// copy keywords to output
				if ( isKeyword ( prop ) ) {
					output.put ( prop, JsonLdUtils.clone ( element.get ( prop ) ) );
					continue;
				}

				// if property isn't in the frame
				if ( !frame.containsKey ( prop ) ) {
					// if explicit is off, embed values
					if ( !explicicOn )
						embedValues ( state, element, prop, output );
					continue;
				}

				// add objects
				final List<Object> value = ( List<Object> ) element.get ( prop );

				for ( final Object item : value ) {

					// recurse into list
					if ( ( item->MAP() )
					        && ( ( Map<String, Object> ) item ).containsKey ( "@list" ) ) {
						// add empty list
						final Map<String, Object> list = newMap();
						list.put ( "@list", new ArrayList<Object>() );
						addFrameOutput ( state, output, prop, list );

						// add list objects
						for ( final Object listitem : ( List<Object> ) ( ( Map<String, Object> ) item )
						        .get ( "@list" ) ) {
							// recurse into subject reference
							if ( JsonLdUtils.isNodeReference ( listitem ) ) {
								final Map<String, Object> tmp = newMap();
								final String itemid = ( String ) ( ( Map<String, Object> ) listitem )
								                      .get ( "@id" );
								// TODO: nodes may need to be node_map,
								// which is global
								tmp.put ( itemid, this.nodeMap.get ( itemid ) );
								frame ( state, tmp,
								        ( Map<String, Object> ) ( ( List<Object> ) frame.get ( prop ) )
								        .get ( 0 ), list, "@list" );
							} else {
								// include other values automatcially (TODO:
								// may need JsonLdUtils.clone(n))
								addFrameOutput ( state, list, "@list", listitem );
							}
						}
					}

					// recurse into subject reference
					else if ( JsonLdUtils.isNodeReference ( item ) ) {
						final Map<String, Object> tmp = newMap();
						final String itemid = ( String ) ( ( Map<String, Object> ) item ).get ( "@id" );
						// TODO: nodes may need to be node_map, which is
						// global
						tmp.put ( itemid, this.nodeMap.get ( itemid ) );
						frame ( state, tmp,
						        ( Map<String, Object> ) ( ( List<Object> ) frame.get ( prop ) ).get ( 0 ),
						        output, prop );
					} else {
						// include other values automatically (TODO: may
						// need JsonLdUtils.clone(o))
						addFrameOutput ( state, output, prop, item );
					}
				}
			}

			// handle defaults
			props = new ArrayList<String> ( frame.keySet() );
			Collections.sort ( props );
			for ( final String prop : props ) {
				// skip keywords
				if ( isKeyword ( prop ) )
					continue;

				final List<Object> pf = ( List<Object> ) frame.get ( prop );
				Map<String, Object> propertyFrame = pf.size() > 0 ? ( Map<String, Object> ) pf
				                                    .get ( 0 ) : null;
				if ( propertyFrame == null )
					propertyFrame = newMap();
				final boolean omitDefaultOn = getFrameFlag ( propertyFrame, "@omitDefault",
				                              state.omitDefault );
				if ( !omitDefaultOn && !output.containsKey ( prop ) ) {
					Object def = "@null";
					if ( propertyFrame.containsKey ( "@default" ) )
						def = JsonLdUtils.clone ( propertyFrame.get ( "@default" ) );
					if ( ! ( def->LIST() ) ) {
						final List<Object> tmp = new ArrayList<Object>();
						tmp.add ( def );
						def = tmp;
					}
					final Map<String, Object> tmp1 = newMap ( "@preserve", def );
					final List<Object> tmp2 = new ArrayList<Object>();
					tmp2.add ( tmp1 );
					output.put ( prop, tmp2 );
				}
			}

			// add output to parent
			addFrameOutput ( state, parent, property, output );
		}
	}
}

private Boolean getFrameFlag ( Map<String, Object> frame, String name, boolean thedefault ) {
	Object value = frame.get ( name );
	if ( value->LIST() ) {
		if ( ( ( List<Object> ) value ).size() > 0 )
			value = ( ( List<Object> ) value ).get ( 0 );
	}
	if ( value->MAP() && ( ( Map<String, Object> ) value ).containsKey ( "@value" ) )
		value = ( ( Map<String, Object> ) value ).get ( "@value" );
	if ( value instanceof Boolean )
		return ( Boolean ) value;
	return thedefault;
}

/**
    Removes an existing embed.

    @param state
              the current framing state.
    @param id
              the @id of the embed to remove.
*/
private static void removeEmbed ( FramingContext state, String id ) {
	// get existing embed
	final Map<String, EmbedNode> embeds = state.embeds;
	final EmbedNode embed = embeds.get ( id );
	final Object parent = embed.parent;
	final String property = embed.property;

	// create reference to replace embed
	final Map<String, Object> node = newMap ( "@id", id );

	// remove existing embed
	if ( JsonLdUtils.isNode ( parent ) ) {
		// replace subject with reference
		final List<Object> newvals = new ArrayList<Object>();
		final List<Object> oldvals = ( List<Object> ) ( ( Map<String, Object> ) parent )
		                             .get ( property );
		for ( final Object v : oldvals ) {
			if ( v->MAP() && Obj.equals ( ( ( Map<String, Object> ) v ).get ( "@id" ), id ) )
				newvals.add ( node ); else
				newvals.add ( v );
		}
		( ( Map<String, Object> ) parent ).put ( property, newvals );
	}
	// recursively remove dependent dangling embeds
	removeDependents ( embeds, id );
}

private static void removeDependents ( Map<String, EmbedNode> embeds, String id ) {
	// get embed keys as a separate array to enable deleting keys in map
	for ( final String id_dep : embeds.keySet() ) {
		final EmbedNode e = embeds.get ( id_dep );
		final Object p = e.parent != null ? e.parent : newMap();
		if ( ! ( p->MAP() ) )
			continue;
		final String pid = ( String ) ( ( Map<String, Object> ) p ).get ( "@id" );
		if ( Obj.equals ( id, pid ) ) {
			embeds.remove ( id_dep );
			removeDependents ( embeds, id_dep );
		}
	}
}

private Map<String, Object> filterNodes ( FramingContext state, Map<String, Object> nodes,
        Map<String, Object> frame ) throws JsonLdError {
	final Map<String, Object> rval = newMap();
	for ( final String id : nodes.keySet() ) {
		final Map<String, Object> element = ( Map<String, Object> ) nodes.get ( id );
		if ( element != null && filterNode ( state, element, frame ) )
			rval.put ( id, element );
	}
	return rval;
}

private boolean filterNode ( FramingContext state, Map<String, Object> node,
                             Map<String, Object> frame ) throws JsonLdError {
	final Object types = frame.get ( "@type" );
	if ( types != null ) {
		if ( ! ( types->LIST() ) )
			throw new JsonLdError ( Error.SYNTAX_ERROR, "frame @type must be an array" );
		Object nodeTypes = node.get ( "@type" );
		if ( nodeTypes == null )
			nodeTypes = new ArrayList<Object>(); else if ( ! ( nodeTypes->LIST() ) )
			throw new JsonLdError ( Error.SYNTAX_ERROR, "node @type must be an array" );
		if ( ( ( List<Object> ) types ).size() == 1 && ( ( List<Object> ) types ).get ( 0 )->MAP()
		&& ( ( Map<String, Object> ) ( ( List<Object> ) types ).get ( 0 ) ).size() == 0 )
			return ! ( ( List<Object> ) nodeTypes ).isEmpty(); else {
			for ( final Object i : ( List<Object> ) nodeTypes ) {
				for ( final Object j : ( List<Object> ) types ) {
					if ( JsonLdUtils.deepCompare ( i, j ) )
						return true;
				}
			}
			return false;
		}
	} else {
		for ( final String key : frame.keySet() ) {
			if ( "@id".equals ( key ) || !isKeyword ( key ) && ! ( node.containsKey ( key ) ) )
				return false;
		}
		return true;
	}
}

/**
    Adds framing output to the given parent.

    @param state
              the current framing state.
    @param parent
              the parent to add to.
    @param property
              the parent property.
    @param output
              the output to add.
*/
private static void addFrameOutput ( FramingContext state, Object parent, String property,
                                     Object output ) {
	if ( parent->MAP() ) {
		List<Object> prop = ( List<Object> ) ( ( Map<String, Object> ) parent ).get ( property );
		if ( prop == null ) {
			prop = new ArrayList<Object>();
			( ( Map<String, Object> ) parent ).put ( property, prop );
		}
		prop.add ( output );
	} else
		( ( List ) parent ).add ( output );
}

/**
    Embeds values for the given subject and property into the given output
    during the framing algorithm.

    @param state
              the current framing state.
    @param element
              the subject.
    @param property
              the property.
    @param output
              the output.
*/
private void embedValues ( FramingContext state, Map<String, Object> element, String property,
                           Object output ) {
	// embed subject properties in output
	final List<Object> objects = ( List<Object> ) element.get ( property );
	for ( Object o : objects ) {
		// handle subject reference
		if ( JsonLdUtils.isNodeReference ( o ) ) {
			final String sid = ( String ) ( ( Map<String, Object> ) o ).get ( "@id" );

			// embed full subject if isn't already embedded
			if ( !state.embeds.containsKey ( sid ) ) {
				// add embed
				final EmbedNode embed = new EmbedNode();
				embed.parent = output;
				embed.property = property;
				state.embeds.put ( sid, embed );

				// recurse into subject
				o = newMap();
				Map<String, Object> s = ( Map<String, Object> ) this.nodeMap.get ( sid );
				if ( s == null )
					s = newMap ( "@id", sid );
				for ( final String prop : s.keySet() ) {
					// copy keywords
					if ( isKeyword ( prop ) ) {
						( ( Map<String, Object> ) o ).put ( prop, JsonLdUtils.clone ( s.get ( prop ) ) );
						continue;
					}
					embedValues ( state, s, prop, o );
				}
			}
			addFrameOutput ( state, output, property, o );
		}
		// copy non-subject value
		else
			addFrameOutput ( state, output, property, JsonLdUtils.clone ( o ) );
	}
}

/***
    ____ _ __ ____ ____ _____ _ _ _ _ _ / ___|___ _ ____ _____ _ __| |_ / _|_
    __ ___ _ __ ___ | _ \| _ \| ___| / \ | | __ _ ___ _ __(_) |_| |__ _ __
    ___ | | / _ \| '_ \ \ / / _ \ '__| __| | |_| '__/ _ \| '_ ` _ \ | |_) | |
    | | |_ / _ \ | |/ _` |/ _ \| '__| | __| '_ \| '_ ` _ \ | |__| (_) | | | \
    V / __/ | | |_ | _| | | (_) | | | | | | | _ <| |_| | _| / ___ \| | (_| |
    (_) | | | | |_| | | | | | | | | \____\___/|_| |_|\_/ \___|_| \__| |_| |_|
    \___/|_| |_| |_| |_| \_\____/|_| /_/ \_\_|\__, |\___/|_| |_|\__|_| |_|_|
    |_| |_| |___/
*/

/**
    Helper class for node usages

    @author tristan
*/
private class UsagesNode {
	public UsagesNode ( NodeMapNode node, String property, Map<String, Object> value ) {
		this.node = node;
		this.property = property;
		this.value = value;
	}

	public NodeMapNode node = null;
	public String property = null;
	public Map<String, Object> value = null;
}

private class NodeMapNode extends LinkedHashMap<String, Object> {
	public List<UsagesNode> usages = new ArrayList();

	public NodeMapNode ( String id ) {
		super();
		this.put ( "@id", id );
	}

	// helper fucntion for 4.3.3
	public boolean isWellFormedListNode() {
		if ( usages.size() != 1 )
			return false;
		int keys = 0;
		if ( containsKey ( RDF_FIRST ) ) {
			keys++;
			if ( ! ( get ( RDF_FIRST )->LIST() && ( ( List<Object> ) get ( RDF_FIRST ) ).size() == 1 ) )
				return false;
		}
		if ( containsKey ( RDF_REST ) ) {
			keys++;
			if ( ! ( get ( RDF_REST )->LIST() && ( ( List<Object> ) get ( RDF_REST ) ).size() == 1 ) )
				return false;
		}
		if ( containsKey ( "@type" ) ) {
			keys++;
			if ( ! ( get ( "@type" )->LIST() && ( ( List<Object> ) get ( "@type" ) ).size() == 1 )
			        && RDF_LIST.equals ( ( ( List<Object> ) get ( "@type" ) ).get ( 0 ) ) )
				return false;
		}
		// TODO: SPEC: 4.3.3 has no mention of @id
		if ( containsKey ( "@id" ) )
			keys++;
		if ( keys < size() )
			return false;
		return true;
	}

	// return this node without the usages variable
	public Map<String, Object> serialize() {
		return new LinkedHashMap<String, Object> ( this );
	}
}

/**
    Converts RDF statements into JSON-LD.

    @param dataset
              the RDF statements.
    @return A list of JSON-LD objects found in the given dataset.
    @throws JsonLdError
               If there was an error during conversion from RDF to JSON-LD.
*/
public List<Object> fromRDF ( final RDFDataset dataset ) throws JsonLdError {
	// 1)
	final Map<String, NodeMapNode> defaultGraph = new LinkedHashMap<String, NodeMapNode>();
	// 2)
	final Map<String, Map<String, NodeMapNode>> graphMap = new LinkedHashMap<String, Map<String, NodeMapNode>>();
	graphMap.put ( "@default", defaultGraph );

	// 3/3.1)
	for ( final String name : dataset.graphNames() ) {

		final List<RDFDataset.Quad> graph = dataset.getQuads ( name );

		// 3.2+3.4)
		Map<String, NodeMapNode> nodeMap;
		if ( !graphMap.containsKey ( name ) ) {
			nodeMap = new LinkedHashMap<String, NodeMapNode>();
			graphMap.put ( name, nodeMap );
		} else
			nodeMap = graphMap.get ( name );

		// 3.3)
		if ( !"@default".equals ( name ) && !Obj.contains ( defaultGraph, name ) )
			defaultGraph.put ( name, new NodeMapNode ( name ) );

		// 3.5)
		for ( final RDFDataset.Quad triple : graph ) {
			final String subject = triple.getSubject().getValue();
			final String predicate = triple.getPredicate().getValue();
			final RDFDataset.Node object = triple.getObject();

			// 3.5.1+3.5.2)
			NodeMapNode node;
			if ( !nodeMap.containsKey ( subject ) ) {
				node = new NodeMapNode ( subject );
				nodeMap.put ( subject, node );
			} else
				node = nodeMap.get ( subject );

			// 3.5.3)
			if ( ( object.isIRI() || object.isBlankNode() )
			        && !nodeMap.containsKey ( object.getValue() ) )
				nodeMap.put ( object.getValue(), new NodeMapNode ( object.getValue() ) );

			// 3.5.4)
			if ( RDF_TYPE.equals ( predicate ) && ( object.isIRI() || object.isBlankNode() )
			        && !opts.getUseRdfType() ) {
				JsonLdUtils.mergeValue ( node, "@type", object.getValue() );
				continue;
			}

			// 3.5.5)
			final Map<String, Object> value = object.toObject ( opts.getUseNativeTypes() );

			// 3.5.6+7)
			JsonLdUtils.mergeValue ( node, predicate, value );

			// 3.5.8)
			if ( object.isBlankNode() || object.isIRI() ) {
				// 3.5.8.1-3)
				nodeMap.get ( object.getValue() ).usages
				.add ( new UsagesNode ( node, predicate, value ) );
			}
		}
	}

	// 4)
	for ( final String name : graphMap.keySet() ) {
		final Map<String, NodeMapNode> graph = graphMap.get ( name );

		// 4.1)
		if ( !graph.containsKey ( RDF_NIL ) )
			continue;

		// 4.2)
		final NodeMapNode nil = graph.get ( RDF_NIL );
		// 4.3)
		for ( final UsagesNode usage : nil.usages ) {
			// 4.3.1)
			NodeMapNode node = usage.node;
			String property = usage.property;
			Map<String, Object> head = usage.value;
			// 4.3.2)
			final List<Object> list = new ArrayList<Object>();
			final List<String> listNodes = new ArrayList<String>();
			// 4.3.3)
			while ( RDF_REST.equals ( property ) && node.isWellFormedListNode() ) {
				// 4.3.3.1)
				list.add ( ( ( List<Object> ) node.get ( RDF_FIRST ) ).get ( 0 ) );
				// 4.3.3.2)
				listNodes.add ( ( String ) node.get ( "@id" ) );
				// 4.3.3.3)
				final UsagesNode nodeUsage = node.usages.get ( 0 );
				// 4.3.3.4)
				node = nodeUsage.node;
				property = nodeUsage.property;
				head = nodeUsage.value;
				// 4.3.3.5)
				if ( !JsonLdUtils.isBlankNode ( node ) )
					break;
			}
			// 4.3.4)
			if ( RDF_FIRST.equals ( property ) ) {
				// 4.3.4.1)
				if ( RDF_NIL.equals ( node.get ( "@id" ) ) )
					continue;
				// 4.3.4.3)
				final String headId = ( String ) head.get ( "@id" );
				// 4.3.4.4-5)
				head = ( Map<String, Object> ) ( ( List<Object> ) graph.get ( headId ).get ( RDF_REST ) )
				       .get ( 0 );
				// 4.3.4.6)
				list.remove ( list.size() - 1 );
				listNodes.remove ( listNodes.size() - 1 );
			}
			// 4.3.5)
			head.remove ( "@id" );
			// 4.3.6)
			Collections.reverse ( list );
			// 4.3.7)
			head.put ( "@list", list );
			// 4.3.8)
			for ( final String nodeId : listNodes )
				graph.remove ( nodeId );
		}
	}

	// 5)
	final List<Object> result = new ArrayList<Object>();
	// 6)
	final List<String> ids = new ArrayList<String> ( defaultGraph.keySet() );
	Collections.sort ( ids );
	for ( final String subject : ids ) {
		final NodeMapNode node = defaultGraph.get ( subject );
		// 6.1)
		if ( graphMap.containsKey ( subject ) ) {
			// 6.1.1)
			node.put ( "@graph", new ArrayList<Object>() );
			// 6.1.2)
			final List<String> keys = new ArrayList<String> ( graphMap.get ( subject ).keySet() );
			Collections.sort ( keys );
			for ( final String s : keys ) {
				final NodeMapNode n = graphMap.get ( subject ).get ( s );
				if ( n.size() == 1 && n.containsKey ( "@id" ) )
					continue;
				( ( List<Object> ) node.get ( "@graph" ) ).add ( n.serialize() );
			}
		}
		// 6.2)
		if ( node.size() == 1 && node.containsKey ( "@id" ) )
			continue;
		result.add ( node.serialize() );
	}

	return result;
}

/***
    ____ _ _ ____ ____ _____ _ _ _ _ _ / ___|___ _ ____ _____ _ __| |_ | |_
    ___ | _ \| _ \| ___| / \ | | __ _ ___ _ __(_) |_| |__ _ __ ___ | | / _ \|
    '_ \ \ / / _ \ '__| __| | __/ _ \ | |_) | | | | |_ / _ \ | |/ _` |/ _ \|
    '__| | __| '_ \| '_ ` _ \ | |__| (_) | | | \ V / __/ | | |_ | || (_) | |
    _ <| |_| | _| / ___ \| | (_| | (_) | | | | |_| | | | | | | | |
    \____\___/|_| |_|\_/ \___|_| \__| \__\___/ |_| \_\____/|_| /_/ \_\_|\__,
    |\___/|_| |_|\__|_| |_|_| |_| |_| |___/
*/

/**
    Adds RDF triples for each graph in the current node map to an RDF
    dataset.

    @return the RDF dataset.
    @throws JsonLdError
               If there was an error converting from JSON-LD to RDF.
*/
public RDFDataset toRDF() throws JsonLdError {
	// TODO: make the default generateNodeMap call (i.e. without a
	// graphName) create and return the nodeMap
	final Map<String, Object> nodeMap = newMap();
	nodeMap.put ( "@default", newMap() );
	generateNodeMap ( this.value, nodeMap );

	final RDFDataset dataset = new RDFDataset ( this );

	for ( final String graphName : nodeMap.keySet() ) {
		// 4.1)
		if ( JsonLdUtils.isRelativeIri ( graphName ) )
			continue;
		final Map<String, Object> graph = ( Map<String, Object> ) nodeMap.get ( graphName );
		dataset.graphToRDF ( graphName, graph );
	}

	return dataset;
}

/***
    _ _ _ _ _ _ _ _ _ _ _ | \ | | ___ _ __ _ __ ___ __ _| (_)______ _| |_(_)
    ___ _ __ / \ | | __ _ ___ _ __(_) |_| |__ _ __ ___ | \| |/ _ \| '__| '_ `
    _ \ / _` | | |_ / _` | __| |/ _ \| '_ \ / _ \ | |/ _` |/ _ \| '__| | __|
    '_ \| '_ ` _ \ | |\ | (_) | | | | | | | | (_| | | |/ / (_| | |_| | (_) |
    | | | / ___ \| | (_| | (_) | | | | |_| | | | | | | | | |_| \_|\___/|_|
    |_| |_| |_|\__,_|_|_/___\__,_|\__|_|\___/|_| |_| /_/ \_\_|\__, |\___/|_|
    |_|\__|_| |_|_| |_| |_| |___/
*/

/**
    Performs RDF normalization on the given JSON-LD input.

    @param dataset
              the expanded JSON-LD object to normalize.
    @return The normalized JSON-LD object
    @throws JsonLdError
               If there was an error while normalizing.
*/
public Object normalize ( Map<String, Object> dataset ) throws JsonLdError {
	// create quads and map bnodes to their associated quads
	final List<Object> quads = new ArrayList<Object>();
	final Map<String, Object> bnodes = newMap();
	for ( String graphName : dataset.keySet() ) {
		final List<Map<String, Object>> triples = ( List<Map<String, Object>> ) dataset
		.get ( graphName );
		if ( "@default".equals ( graphName ) )
			graphName = null;
		for ( final Map<String, Object> quad : triples ) {
			if ( graphName != null ) {
				if ( graphName.indexOf ( "_:" ) == 0 ) {
					final Map<String, Object> tmp = newMap();
					tmp.put ( "type", "blank node" );
					tmp.put ( "value", graphName );
					quad.put ( "name", tmp );
				} else {
					final Map<String, Object> tmp = newMap();
					tmp.put ( "type", "IRI" );
					tmp.put ( "value", graphName );
					quad.put ( "name", tmp );
				}
			}
			quads.add ( quad );

			final String[] attrs = new String[] { "subject", "object", "name" };
			for ( final String attr : attrs ) {
				if ( quad.containsKey ( attr )
				        && "blank node".equals ( ( ( Map<String, Object> ) quad.get ( attr ) )
				                                 .get ( "type" ) ) ) {
					final String id = ( String ) ( ( Map<String, Object> ) quad.get ( attr ) )
					                  .get ( "value" );
					if ( !bnodes.containsKey ( id ) ) {
						bnodes.put ( id, new LinkedHashMap<String, List<Object>>() {
							{
								put ( "quads", new ArrayList<Object>() );
							}
						} );
					}
					( ( List<Object> ) ( ( Map<String, Object> ) bnodes.get ( id ) ).get ( "quads" ) )
					.add ( quad );
				}
			}
		}
	}

	// mapping complete, start canonical naming
	final NormalizeUtils normalizeUtils = new NormalizeUtils ( quads, bnodes, new UniqueNamer (
	            "_:c14n" ), opts );
	return normalizeUtils.hashBlankNodes ( bnodes.keySet() );
}

}
#endif
