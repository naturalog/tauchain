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
		if ( context ) context = context->parse ( context_ );
	}
public:
	// http://json-ld.org/spec/latest/json-ld-api/#compaction-algorithm
	pobj compact ( pcontext activeCtx, string activeProperty, pobj element, bool compactArrays ) {
		if ( element->LIST() ) {
			polist result = mk_olist();
			for ( pobj item : *element->LIST() ) {
				pobj compactedItem = compact ( activeCtx, activeProperty, item, compactArrays );
				if ( compactedItem ) result->push_back ( compactedItem );
			}
			return ( compactArrays && result->size() == 1 && !activeCtx->getContainer ( activeProperty ) ) ? result->at ( 0 ) : mk_olist_obj ( result );
		}
		if ( !element->MAP() ) return element;

		psomap elem = element->MAP();
		if ( has ( elem, "@value" ) || has ( elem, "@id" ) )
			// TODO: spec tells to pass also inverse to compactValue
			if ( pobj compacted_val = activeCtx->compactValue ( activeProperty, mk_somap_obj ( elem ) ) )
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
				if ( exp_val->STR() ) compacted_val = make_shared<string_obj> ( activeCtx->compactIri ( exp_val->STR(), exp_prop == "@type" ) );
				else {
					vector<string> types;
					for ( auto expandedType : *exp_val->LIST() ) types.push_back ( *activeCtx->compactIri ( expandedType->STR(), true ) );
					if ( types.size() == 1 ) compacted_val = make_shared<string_obj> ( types[0] );
					else compacted_val = mk_olist_obj ( vec2vec ( types ) );
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
							value = mk_olist_obj ( olist ( 1, value ) );
						if ( !has ( result, property ) ) result->at ( property ) = value;
						else {
							make_list_if_not ( result->at ( property ) );
							add_all ( result->at ( property )->LIST(), value );
						}
						compacted_val->erase ( property );
					}
				}
				if ( compacted_val->size() ) result->at ( *activeCtx->compactIri ( "@reverse", true ) ) = mk_somap_obj ( compacted_val );
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
				if ( it == result->end() ) result->at ( itemActiveProperty ) = mk_olist_obj();
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
						compactedItem = mk_somap_obj ( wrapper );
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
					if ( has ( result, itemActiveProperty ) ) mapObject = mk_somap_obj ( result->at ( itemActiveProperty )->MAP() );
					else result->at ( itemActiveProperty ) = mapObject = mk_somap_obj();
					if ( container == "@language" && has ( compactedItem->MAP(), "@value" ) )
						compactedItem = compactedItem->MAP()->at ( "@value" );
					string mapKey = *exp_item->MAP() ->at ( container )->STR();
					if ( !has ( mapObject->MAP(), mapKey ) ) mapObject->MAP()->at ( mapKey ) = compactedItem;
					else {
						make_list_if_not ( mapObject->MAP()->at ( mapKey ) );
						mapObject->MAP()->at ( mapKey )->LIST()->push_back ( compactedItem );
					}
				}
				else {
					bool check = ( !compactArrays || is ( container, {"@set"s, "@list"} ) || is ( exp_prop, {"@list"s, "@graph"s} ) ) && ( !compactedItem->LIST() );
					if ( check ) compactedItem = mk_olist_obj ( olist ( 1, compactedItem ) );
					if ( !has ( result, itemActiveProperty ) )  result->at ( itemActiveProperty ) = compactedItem;
					else {
						make_list_if_not ( result->at ( itemActiveProperty ) );
						add_all ( result->at ( itemActiveProperty )->LIST(), compactedItem );
					}
				}
			}
		}
		return mk_somap_obj ( result );
	}

	pobj compact ( pcontext activeCtx, string activeProperty, pobj element ) {
		return compact ( activeCtx, activeProperty, element, true );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm
	pobj expand ( pcontext& activeCtx, const string& activeProperty, pobj& element ) {
		if ( !element )  return 0;
		if ( element->LIST() ) {
			polist_obj result = mk_olist_obj();
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
						exp_val = make_shared<string_obj> ( activeCtx->expandIri ( value->STR(), true, false, 0, 0 ) );
					} else if ( *exp_prop == "@type" ) {
						if ( value->LIST() ) {
							exp_val = mk_olist_obj();
							for ( pobj v :  *value->LIST() ) {
								if ( !v->STR() ) throw INVALID_TYPE_VALUE + "\t" + "@type value must be a string or array of strings" ;
								exp_val->LIST()->push_back ( make_shared<string_obj> ( activeCtx->expandIri ( v->STR(), true, true, 0, 0 ) ) );
							}
						} else if ( value->STR() ) exp_val = make_shared<string_obj> ( activeCtx->expandIri ( value->STR(), true, true, 0, 0 ) );
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
						if ( !exp_val->LIST() ) exp_val = mk_olist_obj ( olist ( 1, exp_val ) );
						for ( auto o : *exp_val->LIST() ) if ( o->MAP() && has ( o->MAP(), "@list" ) ) throw LIST_OF_LISTS + "\t" + "A list may not contain another list";
					} else if ( *exp_prop == "@set" ) exp_val = expand ( activeCtx, activeProperty, value );
					else if ( *exp_prop == "@reverse" ) {
						if ( !value->MAP() ) throw INVALID_REVERSE_VALUE + "\t" + "@reverse value must be an object";
						exp_val = expand ( activeCtx, "@reverse", value );
						if ( has ( exp_val->MAP(), "@reverse" ) ) {
							psomap reverse = exp_val->MAP()->at ( "@reverse" )->MAP();
							for ( auto z : *reverse ) {
								string property = z.first;
								pobj item = z.second;
								if ( !has ( result, property ) ) result->at ( property ) = mk_olist_obj();
								add_all ( result->at ( property )->LIST(), item );
							}
						}
						if ( exp_val->MAP()->size() > ( has ( exp_val->MAP(), "@reverse" ) ? 1 : 0 ) ) {
							if ( !has ( result, "@reverse" ) )  result->at ( "@reverse" ) = mk_somap_obj();
							psomap reverseMap = result->at ( "@reverse" )->MAP();
							for ( auto t : *exp_val->MAP() ) {
								string property = t.first;
								if ( property == "@reverse" ) continue;
								polist items = exp_val->MAP()->at ( property )->LIST();
								for ( pobj item : *items ) {
									if ( has ( item->MAP(), "@value" ) || has ( item->MAP(), "@list" ) ) throw INVALID_REVERSE_PROPERTY_VALUE;
									if ( !has ( reverseMap, property ) ) reverseMap->at ( property ) = mk_olist_obj();
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
					exp_val = mk_olist_obj();
					for ( auto yy : *value->MAP() ) {
						string language = yy.first;
						pobj languageValue = yy.second;
						make_list_if_not ( languageValue );
						for ( pobj item : *languageValue->LIST() ) {
							if ( ! item->STR() ) throw INVALID_LANGUAGE_MAP_VALUE; // + "\t" + "Expected " + item.toString() + " to be a string");
							somap tmp;
							tmp["@value"] = item;
							tmp["@language"] = make_shared<string_obj> ( lower ( language ) );
							exp_val->LIST( )->push_back ( mk_somap_obj ( tmp ) );
						}
					}
				} else if ( *activeCtx->getContainer ( key ) == "@index" && value->MAP() ) {
					exp_val = mk_olist_obj();
					for ( auto xx : *value->MAP() ) {
						pobj indexValue = xx.second;
						make_list_if_not ( indexValue );
						indexValue = expand ( activeCtx, key, indexValue );
						for ( pobj item : *indexValue->LIST() ) {
							if ( !has ( item->MAP(), "@index" ) ) item->MAP()->at ( "@index" ) = make_shared<string_obj> ( xx.first );
							exp_val->LIST()->push_back ( item );
						}
					}
				} else exp_val = expand ( activeCtx, key, value );
				if ( !exp_val ) continue;
				if ( *activeCtx->getContainer ( key ) == "@list" && !has ( exp_val->MAP(), "@list" ) ) {
					auto tmp = exp_val;
					make_list_if_not ( tmp );
					exp_val = mk_somap_obj();
					exp_val->MAP( )->at ( "@list" ) = tmp;
				}
				if ( activeCtx->isReverseProperty ( key ) ) {
					if ( !has ( result, "@reverse" ) ) result->at ( "@reverse" ) = mk_somap_obj();
					psomap reverseMap =  result->at ( "@reverse" )->MAP();
					make_list_if_not ( exp_val );
					for ( pobj item : *exp_val->LIST() ) {
						if ( has ( item->MAP(), "@value" ) && has ( item->MAP(), "@list" ) ) throw INVALID_REVERSE_PROPERTY_VALUE;
						if ( !has ( reverseMap, exp_prop ) ) reverseMap->at ( *exp_prop ) = mk_olist_obj();
						add_all ( reverseMap->at ( *exp_prop )->LIST(), item );
					}
				} else {
					if ( !has ( result, exp_prop ) ) result->at ( *exp_prop ) = mk_olist_obj();
					add_all ( result->at ( *exp_prop )->LIST(), exp_val );
				}
			}
			if ( hasvalue ( result ) ) {
				//				 Set<String> keySet = new HashSet ( result.keySet() );
				somap ks ( *result );
				if ( hasvalue ( ks ) ) ks.erase ( "@value" );
				if ( hasindex ( ks ) ) ks.erase ( "@index" );
				bool langremoved = haslang ( ks );
				bool typeremoved = hastype ( ks );
				if ( langremoved ) ks.erase ( "@language" );
				if ( typeremoved ) ks.erase ( "@type" );
				if ( ( langremoved && typeremoved ) || ks.size() ) throw INVALID_VALUE_OBJECT + "\t" + "value object has unknown keys";
				pobj rval = getvalue(result);
				if ( !rval ) return 0;
				if ( ! rval->STR() && haslang ( result ) ) throw INVALID_LANGUAGE_TAGGED_VALUE + "\t" + "when @language is used, @value must be a string";
				else if ( hastype ( result ) )
					if ( ! ( gettype(result )->STR() ) || startsWith ( *gettype(result)->STR(), "_:" ) || gettype(result)->STR()->find ( ":" ) == string::npos )
						throw INVALID_TYPED_VALUE + "\t" + "value of @type must be an IRI";
			} else if ( hastype ( result ) ) {
				pobj rtype = gettype(result);
				if ( !rtype->LIST() ) gettype(result) = mk_olist_obj ( olist ( 1, rtype ) ) ;
			} else if ( hasset ( result ) || haslist ( result ) ) {
				if ( result->size() > ( hasindex ( result ) ? 2 : 1 ) )
					throw INVALID_SET_OR_LIST_OBJECT + "\t" + "@set or @list may only contain @index";
				if ( hasset ( result) ) return getset(result);
			}
			if ( ( haslang ( result ) && result->size() == 1 ) ||  ( activeProperty == "@graph" ) && result && ( ( !result->size() || hasvalue ( result ) || haslist ( result ) ) ) ||
			        ( hasid ( result ) && result->size() == 1 ) ) result = 0;
			return mk_somap_obj ( result );
		}
		if ( activeProperty == "@graph" ) return 0;
		return activeCtx->expandValue ( activeProperty, element );
	}

	static bool deepCompare ( pobj v1, pobj v2, bool listOrderMatters = false ) {
		if ( !v1 ) return !v2;
		if ( !v2 ) return !v1;
		if ( v1->MAP() && v2->MAP() ) {
			psomap m1 = v1->MAP(), m2 = v2->MAP();
			if ( m1->size() != m2->size() ) return false;
			for ( auto x : *m1 ) if ( !has ( m2, x.first ) || !deepCompare ( x.second, m2->at ( x.first ), listOrderMatters ) ) return false;
			return true;
		} else if ( v1->LIST() && v2->LIST() ) {
			polist l1 = v1->LIST(), l2 = v2->LIST();
			if ( l1->size() != l2->size() ) return false;
			// used to mark members of l2 that we have already matched to avoid
			// matching the same item twice for lists that have duplicates
			bool *alreadyMatched = new bool[l2->size()];
			for ( size_t i = 0; i < l1->size(); ++i ) {
				pobj o1 = l1->at ( i );
				bool gotmatch = false;
				if ( listOrderMatters ) gotmatch = deepCompare ( o1, l2->at ( i ), listOrderMatters );
				else for ( int j = 0; j < l2->size(); j++ )
						if ( !alreadyMatched[j] && deepCompare ( o1, l2->at ( j ), listOrderMatters ) ) {
							alreadyMatched[j] = true;
							gotmatch = true;
							break;
						}
				delete[] alreadyMatched;
				if ( !gotmatch ) return false;
			}
			return true;
		} else return equals ( v1, v2 );
	}

	static bool deepContains ( polist values, pobj value ) {
		for ( pobj item : *values ) if ( deepCompare ( item, value, false ) ) return true;
		return false;
	}

	static void mergeValue ( psomap obj, pstring key, pobj value ) {
		if ( obj && key ) mergeValue ( *obj, *key, value );
	}
	static void mergeValue ( psomap obj, string key, pobj value ) {
		if ( obj ) mergeValue ( *obj, key, value );
	}
	static void mergeValue ( somap obj, pstring key, pobj value ) {
		if ( key ) mergeValue ( obj, *key, value );
	}

	static void mergeValue ( somap obj, string key, pobj value ) {
		polist values = obj[key]->LIST();
		if ( !values ) obj[key] = mk_olist_obj(values = mk_olist());
		if ( key == "@list" || ( value->MAP() && has ( value->MAP(), "@list" ) ) || !deepContains ( values, value ) )
			values->push_back ( value );
	}

	void generateNodeMap ( pobj element, psomap nodeMap )  {
		generateNodeMap ( element, nodeMap, "@default", 0, 0, 0 );
	}

	void generateNodeMap ( pobj element, psomap nodeMap, string activeGraph ) {
		generateNodeMap ( element, nodeMap, activeGraph, 0, 0, 0 );
	}

	string gen_bnode_id ( string id = "" ) {
		if ( has ( bnode_id_map, id ) ) return bnode_id_map[id];
		string bnid = "_:b" + blankNodeCounter++;
		bnode_id_map[id] = bnid;
	}

	size_t blankNodeCounter = 0;
	map<string, string> bnode_id_map;

	void generateNodeMap ( pobj element, psomap nodeMap, string activeGraph, pobj activeSubject, pstring activeProperty, psomap list ) {
		if ( element->LIST() ) {
			for ( pobj item : *element->LIST() ) generateNodeMap ( item, nodeMap, activeGraph, activeSubject, activeProperty, list );
			return;
		}
		psomap elem = element->MAP();
		if ( !has ( nodeMap, activeGraph ) ) nodeMap->at ( activeGraph ) = mk_somap_obj();
		psomap graph = nodeMap->at ( activeGraph )->MAP(), node = activeSubject ? graph->at ( *activeSubject->STR() )->MAP() : 0;
		if ( hastype ( elem ) ) {
			vector<string> oldTypes, newTypes;
			if ( gettype(elem )->LIST() ) oldTypes = vec2vec ( gettype(elem)->LIST() );
			else {
				oldTypes = vector<string>();//mk_olist_obj();
				oldTypes.push_back ( *elem->at ( "@type" )->STR() );
			}
			for ( string item : oldTypes ) {
				if ( startsWith ( item, "_:" ) ) newTypes.push_back ( gen_bnode_id ( item ) );
				else newTypes.push_back ( item );
			}
			if ( gettype(elem)->LIST() ) gettype(elem) = mk_olist_obj(vec2vec ( newTypes ));
			else gettype(elem) = make_shared<string_obj> ( newTypes[0] );
		}
		if ( hasvalue ( elem ) ) {
			if ( !list ) mergeValue ( node, activeProperty, element );
			else mergeValue ( list, "@list", element );
		} else if ( haslist ( elem ) ) {
			psomap result = make_shared<somap>();
			( *result ) [ "@list"] = mk_olist_obj();
			generateNodeMap ( getlist(elem), nodeMap, activeGraph, activeSubject, activeProperty, result );
			mergeValue ( node, activeProperty, mk_somap_obj ( result ) );
		} else {
			string id;
			if ( hasid ( elem ) && getid(elem)->STR() ) {
				string id = *elem->at ( "@id" )->STR();
				elem->erase ( "@id" );
				if ( startsWith ( id, "_:" ) ) id = gen_bnode_id ( id );
			} else id = gen_bnode_id ( );
			if ( !has ( graph, id ) ) {
				somap tmp;
				tmp[ "@id"] = make_shared<string_obj> ( id );
				graph->at ( id ) = mk_somap_obj ( tmp );
			}
			if ( activeSubject->MAP() ) mergeValue ( graph->at ( id )->MAP(), activeProperty, activeSubject );
			else if ( activeProperty ) {
				somap ref;
				ref[ "@id"] = make_shared<string_obj> ( id );
				if ( !list ) mergeValue ( node, activeProperty, mk_somap_obj ( ref ) );
				else mergeValue ( list, "@list", mk_somap_obj ( ref ) );
			}
			node = graph->at ( id )->MAP();
			if ( hastype ( elem ) ) {
				for ( pobj type : *gettype(elem)->LIST() ) mergeValue ( node, "@type", type );
				elem->erase ( "@type" );
			}
			if ( hasindex ( elem ) ) {
				pobj elemIndex  = getindex(elem);
				elem->erase ( "@index" );
				if ( hasindex ( node ) ) {
					if ( !deepCompare ( getindex(node), elemIndex ) ) throw CONFLICTING_INDEXES;
				} else getindex(node) = elemIndex ;
			}
			if ( hasreverse ( elem ) ) {
				psomap refnode = make_shared<somap>(), revmap = elem->at ( "@reverse" )->MAP();
				( *refnode ) ["@id"] = make_shared<string_obj> ( id );
				elem->erase ( "@reverse" );
				for ( auto x : *revmap ) {
					string prop = x.first;
					polist values = revmap->at ( prop )->LIST();
					for ( pobj value : *values ) generateNodeMap ( value, nodeMap, activeGraph, mk_somap_obj ( refnode ), make_shared<string> ( prop ), 0 );
				}
			}
			if ( has ( elem, "@graph" ) ) {
				generateNodeMap ( elem->at ( "@graph" ), nodeMap, id, 0, 0, 0 );
				elem->erase ( "@graph" );
			}
			//			final List<String> keys = new ArrayList<String> ( elem.keySet() );
			//			Collections.sort ( keys );
			for ( auto z : *elem ) {
				string property = z.first;
				pobj value = z.second;
				if ( startsWith ( property, "_:" ) ) property = gen_bnode_id ( property );
				if ( !has ( node, property ) ) node->at ( property ) = mk_olist_obj();
				generateNodeMap ( value, nodeMap, activeGraph, make_shared<string_obj>(id), make_shared<string>(property), 0 );
			}
		}
	}
};
