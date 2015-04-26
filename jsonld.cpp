#include "jsonld.h"
#include "rdf.h"

namespace jsonld {

pcontext context_t::parse ( pobj localContext, vector<string> remoteContexts ) {
	pdefined_t defined = make_shared<defined_t>();
	context_t result ( options );
	if ( !localContext || !localContext->LIST() ) localContext = mk_olist_obj ( olist ( 1, localContext ) );
	for ( auto context : *localContext->LIST() ) {
		if ( !context || context->Null() ) {
			result = context_t ( options );
			continue;
		}
		if ( context->STR() ) {
			pstring s = context->STR();
			somap& t1 = *result.MAP();
			pobj p1 = sgetbase ( t1 );
			pstring s1 = p1 ? p1->STR() : 0;
			string uri = resolve ( s1, *s );
			if ( std::find ( remoteContexts.begin(), remoteContexts.end(), uri ) != remoteContexts.end() )
				throw std::runtime_error ( RECURSIVE_CONTEXT_INCLUSION + tab + uri );
			remoteContexts.push_back ( uri );
			pobj remoteContext = fromURL ( uri );
			if ( remoteContext && !remoteContext->map_and_has ( str_context ) )
				throw std::runtime_error ( INVALID_REMOTE_CONTEXT + tab + context->toString() );
			context = remoteContext ? ( *remoteContext->MAP() ) [str_context] : 0;
			result = *result.parse ( context, remoteContexts );
			continue;
		}
		if ( !context->MAP() ) throw std::runtime_error ( INVALID_LOCAL_CONTEXT + string ( "\r\n" ) + context->toString() );
		somap& cm = *context->MAP();
		auto it = cm.find ( "@base" );
		if ( !remoteContexts.size() && it != cm.end() ) {
			pobj value = it->second;
			if ( value->Null() ) result.MAP()->erase ( "@base" );
			else if ( pstring s = value->STR() ) {
				if ( is_abs_iri ( *s ) ) ( *result.MAP() ) ["@base"] = value;
				else {
					pstring baseUri = ( *result.MAP() ) ["@base"]->STR();
					if ( !is_abs_iri ( *baseUri ) ) throw std::runtime_error ( INVALID_BASE_IRI + tab + ( baseUri ? *baseUri : string ( "" ) ) );
					( *result.MAP() ) ["@base"] = make_shared<string_obj> ( resolve ( baseUri, *s ) );
				}
			} else throw Ex9;
		}
		// 3.5
		if ( ( it = cm.find ( str_vocab ) ) != cm.end() ) {
			pobj value = it->second;
			if ( value->Null() ) result.MAP()->erase ( it );
			else if ( pstring s = value->STR() ) {
				if ( is_abs_iri ( *s ) ) ( *result.MAP() ) [str_vocab] = value;
				else throw Ex7;
			} else throw Ex8;
		}
		if ( ( it = cm.find ( str_lang ) ) != cm.end() ) {
			pobj value = it->second;
			if ( value->Null() ) result.MAP()->erase ( it );
			else if ( pstring s = value->STR() ) getlang ( result ) = make_shared<string_obj> ( lower ( *s ) );
			else throw std::runtime_error ( INVALID_DEFAULT_LANGUAGE + tab + value->toString() );
		}
		for ( auto it : cm ) {
			if ( is ( it.first, { str_base, str_vocab, str_lang } ) ) continue;
			result.create_term_def ( context->MAP(), it.first, defined ); // REVISE
		}
	}
	return make_shared<context_t> ( result );
}

void context_t::create_term_def ( const psomap context, const string term, pdefined_t pdefined ) {
	defined_t& defined = *pdefined;
	auto dit = defined.find ( term );
	if ( dit != defined.end() ) {
		if ( dit->second ) return;
		throw std::runtime_error ( CYCLIC_IRI_MAPPING + tab + term );
	}
	defined[term] = false;
	if ( keyword ( term ) ) throw std::runtime_error ( KEYWORD_REDEFINITION + tab + term );
	term_defs->erase ( term ); // 4
	auto it = context->find ( term );
	psomap m;
	if ( it == context->end() || it->second->map_and_has_null ( str_id ) ) {
		( *term_defs ) [term] = 0;//make_shared<null_obj>();
		defined[term] = true;
		return;
	}
	somap value;
	if ( it->second->STR() ) value = *newMap ( str_id, it->second )->MAP();
	else if ( auto x = it->second->MAP() ) value = *x;
	else throw std::runtime_error ( INVALID_TERM_DEFINITION );
	somap defn;//, &val = *value->MAP();
	if ( ( it = value.find ( str_type ) ) != value.end() ) { // 10
		if ( !it->second->STR() ) throw std::runtime_error ( INVALID_TYPE_MAPPING );
		string type ( *expand_iri ( it->second->STR(), false, true, context, pdefined ) );
		if ( type != str_id && type != str_vocab && !is_abs_iri ( type ) ) throw std::runtime_error ( INVALID_TYPE_MAPPING + tab + type );
		defn[str_type] = make_shared<string_obj> ( type );
	}
	// 11
	if ( ( it = value.find ( str_reverse ) ) != value.end() ) {
		if ( throw_if_not_contains ( value, str_id, INVALID_REVERSE_PROPERTY ) && !it->second->STR() ) throw Ex5;
		string reverse = *expand_iri ( value.at ( str_reverse )->STR(), false, true, context, pdefined );
		if ( !is_abs_iri ( reverse ) ) throw std::runtime_error ( INVALID_IRI_MAPPING + string ( "Non-absolute @reverse IRI: " ) + reverse );
		defn [str_id] = make_shared<string_obj> ( reverse );
		if ( ( it = value.find ( "@container" ) ) != value.end() && is ( *it->second->STR(), { string ( str_set ), str_index }, Ex6 ) )
			defn ["@container"] = it->second;
		defn[str_reverse] = make_shared<bool_obj> ( ( *pdefined ) [term] = true );
		( *term_defs ) [term] = mk_somap_obj ( defn );
		return;
	}
	defn[str_reverse] = make_shared<bool_obj> ( false );
	size_t colIndex;
	if ( ( it = value.find ( str_id ) ) != value.end() && !it->second->STR ( term ) ) { // 13
		if ( ! it->second->STR() ) throw Ex1;
		pstring res = expand_iri ( it->second->STR(), false, true, context, pdefined );
		if ( res && ( keyword ( *res ) || is_abs_iri ( *res ) ) ) {
			if ( *res == str_context ) throw Ex2;
			defn [str_id] = make_shared<string_obj> ( res );
		} else throw Ex3;
	} else if ( ( ( colIndex = term.find ( ":" ) ) != string::npos ) || ( term.size() && term[0] == '?' ) ) {
		if ( colIndex != string::npos ) {
			string prefix = term.substr ( 0, colIndex ), suffix = term.substr ( colIndex + 1 );
			if ( has ( context, prefix ) ) create_term_def ( context, prefix, pdefined );
			if ( ( it = term_defs->find ( prefix ) ) != term_defs->end() )
				defn [str_id] = make_shared<string_obj> ( *getid ( it->second )->STR() + suffix );
			else defn[str_id] = make_shared<string_obj> ( term );
		} else defn[str_id] = make_shared<string_obj> ( term );
	} else if ( ( it = MAP()->find ( str_vocab ) ) != MAP()->end() )
		defn [str_id] = make_shared<string_obj> ( *MAP()->at ( str_vocab )->STR() + term );
	else throw Ex4;

	// 16
	( ( it = value.find ( "@container" ) ) != value.end() ) && it->second->STR() &&
	is ( *it->second->STR(), { str_list, string ( str_set ), str_index, str_lang }, Ex10 ) && ( defn["@container"] = it->second );

	auto i1 = value.find ( str_lang ), i2 = value.find ( "type" );
	pstring lang;
	if ( i1 != value.end() && i2 == value.end() ) {
		if ( !i1->second->Null() || ( lang = i2->second->STR() ) ) getlang ( defn ) = lang ? make_shared<string_obj> ( lower ( *lang ) ) : 0;
		else throw Ex11;
	}

	( *term_defs ) [term] = mk_somap_obj ( defn );
	( *pdefined ) [term] = true;
}

pstring context_t::expand_iri ( const pstring value, bool relative, bool vocab, const psomap context, pdefined_t defined ) {
	if ( !value || keyword ( *value )/* || ( value->size() && ( *value ) [0] == '?' )*/ ) return value;
	pstring rval;
	if ( has ( context, *value ) && defined->find ( *value ) == defined->end() ) create_term_def ( context, *value, defined );
	somap::iterator it = term_defs->find ( *value );
	if ( vocab && it != term_defs->end() ) {
		if ( auto td = it->second->MAP() ) return ( it = td->find ( str_id ) ) != td->end() ? it->second->STR() : 0;
		return 0;
	} else {
		size_t colIndex = value->find ( ":" );
		if ( colIndex != string::npos ) {
			string prefix = value->substr ( 0, colIndex ), suffix = value->substr ( colIndex + 1 );
			if ( prefix == "_" || startsWith ( suffix, "//" ) ) return value;
			else {
				if ( has ( context, prefix ) && ( defined->find ( prefix ) == defined->end() || !defined->at ( prefix ) ) )
					create_term_def ( context, prefix, defined );
				if ( has ( term_defs, prefix ) ) return pstr ( *term_defs->at ( prefix )->MAP()->at ( str_id )->STR() + suffix );
			}
			return value;
		}
		if ( vocab && has ( MAP(), str_vocab ) ) return pstr ( *MAP()->at ( str_vocab )->STR() + *value );
		else if ( relative ) {
			auto base = get ( MAP(), str_base );
			return pstr ( resolve ( base ? base->STR() : 0, *value ) );
		} else if ( context && is_rel_iri ( *value ) ) throw std::runtime_error ( INVALID_IRI_MAPPING + string ( "not an absolute IRI: " ) + *value );
	}
	return value;
}

psomap_obj context_t::getInverse() {
	if ( inverse ) return inverse;
	inverse = mk_somap_obj();
	pstring defaultLanguage = getlang ( MAP() )->STR();
	if ( !defaultLanguage ) ( *MAP() ) [str_lang] = mk_str_obj ( defaultLanguage = pstr ( "@none" ) );

	for ( auto x : *term_defs ) {
		string term = x.first;
		auto it = term_defs->find ( term );
		psomap definition = it == term_defs->end() || !it->second ? 0 : it->second->MAP();
		if ( !definition ) continue;
		pstring container = ( ( it = definition->find ( "@container" ) ) == definition->end() || !it->second ) ? 0 : it->second->STR();
		if ( !container ) container = pstr ( "@none" );
		pstring iri = ( ( it = definition->find ( str_id ) ) == definition->end() ) || !it->second ? 0 : it->second->STR();

		psomap_obj containerMap = mk_somap_obj ( iri ? inverse->MAP()->at ( *iri )->MAP() : 0 );
		if ( !containerMap ) {
			containerMap = mk_somap_obj();
			inverse->MAP()->at ( *iri ) = containerMap;
		}
		psomap_obj type_lang_map = mk_somap_obj ( container ? containerMap->MAP()->at ( *container )->MAP() : 0 );
		if ( !type_lang_map ) {
			type_lang_map = mk_somap_obj();
			( *type_lang_map->MAP() ) [str_lang] = mk_somap_obj();
			( *type_lang_map->MAP() ) [str_type] = mk_somap_obj();
			( *containerMap->MAP() ) [ *container ] = type_lang_map;
		}
		if ( definition->at ( str_reverse )->BOOL() ) {
			psomap typeMap = type_lang_map->MAP()->at ( str_type )->MAP();
			if ( !hasreverse ( typeMap ) ) ( * typeMap ) [str_reverse] = make_shared<string_obj> ( term );
		} else if ( hastype ( definition ) ) {
			psomap typeMap = gettype ( type_lang_map )->MAP();
			if ( !has ( typeMap, gettype ( definition )->STR() ) ) ( *typeMap ) [ *gettype ( definition )->STR() ] = make_shared<string_obj> ( term );
		} else if ( haslang ( definition ) ) {
			psomap lang_map = gettype ( type_lang_map )->MAP();
			pstring language = getlang ( definition )->STR();
			if ( !language ) ( *definition ) [str_lang] = mk_str_obj ( language = pstr ( "@null" ) );
			if ( !has ( lang_map, language ) ) ( *lang_map ) [ *language ] = make_shared<string_obj> ( term );
		} else {
			psomap lang_map = getlang ( type_lang_map )->MAP();
			if ( !haslang ( lang_map ) ) ( * lang_map ) [str_lang] = make_shared<string_obj> ( term );
			if ( !hasnone ( lang_map ) ) ( * lang_map ) ["@none"] = make_shared<string_obj> ( term );
			psomap typeMap = gettype ( type_lang_map )->MAP();
			if ( !hasnone ( typeMap ) ) ( *typeMap ) ["@none"] = make_shared<string_obj> ( term );
		}
	}
	return inverse;
}

pstring context_t::selectTerm ( string iri, vector<string>& containers, string typeLanguage, vector<string>& preferredValues ) {
	auto inv = getInverse();
	auto containerMap = inv->MAP()->at ( iri )->MAP();
	for ( string container : containers ) {
		if ( !has ( containerMap, container ) ) continue;
		auto type_lang_map = containerMap->at ( container )->MAP();
		auto valueMap = type_lang_map ->at ( typeLanguage )->MAP();
		for ( string item : preferredValues ) {
			if ( !has ( valueMap, item ) ) continue;
			return valueMap->at ( item )->STR();
		}
	}
	return 0;
}

pstring context_t::compactIri ( string iri, pobj value, bool relativeToVocab, bool reverse ) {
	if ( relativeToVocab && has ( inverse->MAP(), iri ) ) {
		auto it = MAP()->find ( str_lang );
		pstring defaultLanguage = 0;
		if ( it != MAP()->end() && it->second ) defaultLanguage = it->second->STR();
		if ( !defaultLanguage ) defaultLanguage = pstr ( "@none" );
		vector<string> containers;
		pstring type_lang = pstr ( str_lang ), type_lang_val = pstr ( "@null" );
		if ( value->MAP() && has ( value->MAP(), str_index ) ) containers.push_back ( str_index );
		if ( reverse ) {
			type_lang = pstr ( str_type );
			type_lang_val = pstr ( str_reverse );
			containers.push_back ( str_set );
		} else if ( value->MAP() && has ( value->MAP(),  str_list ) ) {
			if ( ! has ( value->MAP(),  str_index ) ) containers.push_back ( str_list );
			polist list = value->MAP( )->at ( str_list )->LIST();
			pstring common_lang = ( list->size() == 0 ) ? defaultLanguage : 0, common_type = 0;
			// 2.6.4)
			for ( pobj item : *list ) {
				pstring itemLanguage = pstr ( "@none" ), itemType = pstr ( "@none" );
				if ( isvalue ( item ) ) {
					if ( ( it = item->MAP()->find ( str_lang ) ) != item->MAP()->end() ) itemLanguage = it->second->STR();
					else if (  ( it = item->MAP()->find ( str_type ) ) != item->MAP()->end()  ) itemType = it->second->STR();
					else itemLanguage = pstr ( "@null" );
				} else itemType = pstr ( str_id );
				if ( !common_lang ) common_lang = itemLanguage;
				else if ( common_lang != itemLanguage && isvalue ( item ) ) common_lang = pstr ( "@none" );
				if ( !common_type ) common_type = itemType;
				else if ( common_type != itemType  ) common_type = pstr ( "@none" );
				if ( string ( "@none" ) == *common_lang  && string ( "@none" ) == * common_type  ) break;
			}
			common_lang =  common_lang  ? common_lang : pstr ( "@none" );
			common_type =  common_type  ? common_type : pstr ( "@none" );
			if ( string ( "@none" ) != *common_type )  {
				type_lang = pstr ( str_type );
				type_lang_val = common_type;
			} else type_lang_val = common_lang;
		} else {
			if ( value->MAP() && has ( value->MAP(),  str_value ) ) {
				if ( hasvalue ( value->MAP() )
				        && ! hasindex ( value->MAP() ) ) {
					containers.push_back ( str_lang );
					type_lang_val = getlang ( value )->STR();
				} else if ( hastype ( value->MAP() ) ) {
					type_lang = pstr ( str_type );
					type_lang_val = gettype ( value )->STR();
				}
			} else {
				type_lang = pstr ( str_type );
				type_lang_val = pstr ( str_id );
			}
			containers.push_back ( str_set );
		}

		containers.push_back ( "@none" );
		if ( !type_lang_val ) type_lang_val = pstr ( "@null" );
		vector<string> preferredValues;
		if ( str_reverse ==  *type_lang_val  ) preferredValues.push_back ( str_reverse );
		if ( ( str_reverse ==  *type_lang_val  || str_id ==  *type_lang_val  )
		        && ( value->MAP() ) && has ( value->MAP(),  str_id ) ) {
			pstring result = compactIri (  value->MAP( )->at ( str_id )->STR(), 0, true, true );
			auto it = term_defs->find ( *result );
			if ( it != term_defs->end()
			        && has ( it->second->MAP(), str_id )
			        && jsonld::equals ( value->MAP( )->at ( str_id ),
			                            term_defs->at ( *result )->MAP( )->at ( str_id ) ) ) {
				preferredValues.push_back ( str_vocab );
				preferredValues.push_back ( str_id );
			} else {
				preferredValues.push_back ( str_id );
				preferredValues.push_back ( str_vocab );
			}
		} else preferredValues.push_back ( *type_lang_val );
		preferredValues.push_back ( "@none" );
		pstring term = selectTerm ( iri, containers, *type_lang, preferredValues );
		if ( term  ) return term;
	}
	if ( relativeToVocab && hasvocab ( MAP() ) ) {
		pstring vocab = getvocab ( MAP() )->STR();
		if ( vocab && iri.find ( *vocab ) == 0 && iri != *vocab ) {
			string suffix = iri.substr ( vocab->length() );
			if ( !has ( term_defs, suffix ) ) return pstr ( suffix );
		}
	}
	pstring compactIRI = 0;
	for ( auto x : *term_defs ) {
		string term = x.first;
		psomap term_def = x.second->MAP();
		if ( term.find ( ":" ) != string::npos ) continue;
		if ( !term_def// || iri == term_def->at ( str_id )->STR()
		        || !startsWith ( iri,  *getid ( term_def )->STR() ) )
			continue;

		string candidate = term + ":" + iri.substr ( getid ( term_def )->STR()->length() );
		// TODO: verify && and ||
		if ( ( !compactIRI || compareShortestLeast ( candidate, compactIRI ) < 0 )
		        && ( !has ( term_defs, candidate ) || ( ( iri == *getid ( term_defs->at ( candidate ) )->STR() ) && !value ) ) )
			compactIRI = pstr ( candidate );
	}
	if ( !compactIRI  ) return compactIRI;
	if ( !relativeToVocab ) return removeBase ( getbase ( MAP() ), iri );
	return make_shared<string> ( iri );
}

pobj context_t::compactValue ( string act_prop, psomap_obj value_ ) {
	psomap value = value_->MAP();
	int nvals = value->size();
	auto p = getContainer ( act_prop );
	if ( value->find ( str_index ) != value->end() && p && *p == str_index ) nvals--;
	if ( nvals > 2 ) return value_;//mk_somap_obj ( value_ );
	pstring type_map = get_type_map ( act_prop ), lang_map = get_lang_map ( act_prop );
	auto it = value->find ( str_id );
	if ( it != value->end() ) {
		if ( type_map && *type_map == str_id && nvals == 1 )
			return make_shared<string_obj> ( compactIri ( value->at ( str_id )->STR(), 0, false, false ) );
		else {
			if ( type_map && *type_map == str_vocab && nvals == 1 )
				return make_shared<string_obj> ( compactIri ( value->at ( str_id )->STR(), 0, true, false ) );
		}
		return mk_somap_obj ( value );
	}
	pobj valval = value->at ( str_value );
	it = value->find ( str_type );
	if ( it != value->end() &&  *it->second->STR() == *type_map  ) return valval;
	if ( ( it = value->find ( str_lang ) ) != value->end() ) // TODO: SPEC: doesn't specify to check default language as well
		if ( *it->second->STR() == * lang_map  || jsonld::equals ( it->second, getlang ( MAP() ) ) )
			return valval;
	if ( nvals == 1
	        && ( !valval->STR() || haslang ( MAP() ) || ( term_defs->find ( act_prop ) == term_defs->end()
	                && haslang ( get_term_def ( act_prop ) ) && !lang_map ) ) )
		return valval;
	return value_;
}

pobj context_t::expandValue ( pstring act_prop, pobj value )  {
	somap rval;
	psomap td = has ( term_defs, act_prop ) ? term_defs->at ( *act_prop )->MAP() : 0;
	if ( hastype ( td ) ) {
		if ( *gettype ( td )->STR() == str_id ) {
			rval[ str_id ] = make_shared<string_obj> ( expand_iri ( pstr ( value->toString() ), true, false, 0, 0 ) );
			return mk_somap_obj ( rval );
		}
		if ( *gettype ( td )->STR() == str_vocab ) {
			rval[ str_id ] = make_shared<string_obj> ( expand_iri ( pstr ( value->toString() ), true, true, 0, 0 ) );
			return mk_somap_obj ( rval );
		}
	}
	rval[ str_value ] = value;
	if ( td && hastype ( td ) ) rval[ str_type ] = gettype ( td );
	else if ( value->STR() ) {
		if ( td && haslang ( td ) ) {
			pstring lang = getlang ( td )->STR();
			if ( lang ) rval[ str_lang] = make_shared<string_obj> ( lang );
		} else if ( haslang ( MAP() ) ) rval[ str_lang ] = getlang ( MAP() );
	}
	return mk_somap_obj ( rval );
}

map<string, string> context_t::getPrefixes ( bool onlyCommonPrefixes ) {
	map<string, string> prefixes;
	for ( auto x : *term_defs ) {
		string term = x.first;
		if ( term.find ( ":" ) != string::npos ) continue;
		psomap td = term_defs->at ( term )->MAP();
		if ( !td ) continue;
		pstring id = td->at ( str_id )->STR();
		if ( !id ) continue;
		if ( startsWith ( term, "@" ) || startsWith ( *id, "@" ) ) continue;
		if ( !onlyCommonPrefixes || endsWith ( *id, "/" ) || endsWith ( *id, "#" ) ) prefixes[term] = *id;
	}
	return prefixes;
}

pobj jsonld_api::compact ( pcontext act_ctx, string act_prop, pobj element,
                           bool compactArrays ) {
	if ( element->LIST() ) {
		polist result = mk_olist();
		for ( pobj item : *element->LIST() ) {
			pobj compactedItem = compact ( act_ctx, act_prop, item,
			                               compactArrays );
			if ( compactedItem )
				result->push_back ( compactedItem );
		}
		return ( compactArrays && result->size() == 1
		         && !act_ctx->getContainer ( act_prop ) ) ?
		       result->at ( 0 ) : mk_olist_obj ( result );
	}
	if ( !element->MAP() )
		return element;

	psomap elem = element->MAP();
	if ( has ( elem, str_value ) || has ( elem, str_id ) )
		// TODO: spec tells to pass also inverse to compactValue
		if ( pobj compacted_val = act_ctx->compactValue ( act_prop,
		                          mk_somap_obj ( elem ) ) )
			if ( ! ( compacted_val->MAP() || compacted_val->LIST() ) )
				return compacted_val;

	bool insideReverse = act_prop == str_reverse;
	psomap result = make_shared<somap>();
	for ( auto x : *elem ) { // 7
		string exp_prop = x.first;
		pobj exp_val = x.second;
		if ( is ( exp_prop, { str_id, str_type } ) ) {
			pobj compacted_val;
			// TODO: spec tells to pass also inverse to compactIri
			if ( exp_val->STR() )
				compacted_val = make_shared <string_obj
				                > ( act_ctx->compactIri ( exp_val->STR(),
				                        exp_prop == str_type ) );
			else {
				vector<string> types;
				for ( auto expandedType : *exp_val->LIST() )
					types.push_back (
					    *act_ctx->compactIri ( expandedType->STR(),
					                           true ) );
				if ( types.size() == 1 )
					compacted_val = make_shared <string_obj> ( types[0] );
				else
					compacted_val = mk_olist_obj ( vec2vec ( types ) );
			}
			pstring alias = act_ctx->compactIri ( exp_prop, true );
			result->at ( *alias ) = compacted_val;
			continue;
		}
		if ( exp_prop == str_reverse ) {
			psomap compacted_val = compact ( act_ctx, str_reverse, exp_val,
			                                 compactArrays )->MAP();
			// Must create a new set to avoid modifying the set we are iterating over
			for ( auto y : somap ( *compacted_val ) ) {
				string property = y.first;
				pobj value = y.second;
				if ( act_ctx->isReverseProperty ( property ) ) {
					if ( ( ( act_ctx->getContainer ( property )
					         && *act_ctx->getContainer ( property ) == str_set )
					        || !compactArrays ) && !value->LIST() )
						value = mk_olist_obj ( olist ( 1, value ) );
					if ( !has ( result, property ) )
						( *result ) [property] = value;
					else {
						if ( !result->at ( property )->LIST() )
							( *result ) [property] = mk_olist_obj (
							                             olist ( 1, result->at ( property ) ) );
						add_all ( result->at ( property )->LIST(), value );
					}
					compacted_val->erase ( property );
				}
			}
			if ( compacted_val->size() )
				result->at ( *act_ctx->compactIri ( str_reverse, true ) ) =
				    mk_somap_obj ( compacted_val );
			continue;
		}
		if ( exp_prop == str_index && act_ctx->getContainer ( act_prop )
		        && *act_ctx->getContainer ( act_prop ) == str_index )
			continue;
		if ( is ( exp_prop, { str_index, str_value, str_lang } ) ) {
			result->at ( *act_ctx->compactIri ( exp_prop, true ) ) = exp_val;
			continue;
		}
		if ( !exp_val->LIST()->size() ) {
			string itemActiveProperty = *act_ctx->compactIri ( exp_prop,
			                            exp_val, true, insideReverse );
			auto it = result->find ( itemActiveProperty );
			if ( it == result->end() )
				result->at ( itemActiveProperty ) = mk_olist_obj();
			else if ( !it->second->LIST() )
				it->second = mk_olist_obj ( olist ( 1, it->second ) );

		}
		for ( pobj exp_item : *exp_val->LIST() ) {
			string itemActiveProperty = *act_ctx->compactIri ( exp_prop,
			                            exp_item, true, insideReverse );
			pstring container = act_ctx->getContainer ( itemActiveProperty );
			bool isList = has ( exp_item->MAP(), str_list );
			pobj list = isList ? exp_item->MAP()->at ( str_list ) : 0;
			pobj compactedItem = compact ( act_ctx, itemActiveProperty,
			                               isList ? list : exp_item, compactArrays );
			if ( isList ) {
				if ( !compactedItem->LIST() )
					compactedItem = mk_olist_obj ( olist ( 1, compactedItem ) );
				if ( !container || *container != str_list ) {
					psomap wrapper = make_shared<somap>();
					wrapper->at ( *act_ctx->compactIri ( str_list, true ) ) =
					    compactedItem;
					compactedItem = mk_somap_obj ( wrapper );
					if ( has ( exp_item->MAP(), str_index ) ) {
						compactedItem->MAP()->at (
						    // TODO: SPEC: no mention of vocab =
						    // true
						    *act_ctx->compactIri ( str_index, true ) ) =
						        exp_item->MAP()->at ( str_index );
					}
				} else if ( has ( result, itemActiveProperty ) )
					throw std::runtime_error (
					    COMPACTION_TO_LIST_OF_LISTS + tab
					    + string (
					        "There cannot be two list objects associated with an active property that has a container mapping" ) );
			}
			if ( is ( container, { str_lang, str_index } ) ) {
				psomap_obj mapObject;
				if ( has ( result, itemActiveProperty ) )
					mapObject = mk_somap_obj (
					                result->at ( itemActiveProperty )->MAP() );
				else
					result->at ( itemActiveProperty ) = mapObject =
					                                        mk_somap_obj();
				if ( *container == str_lang
				        && has ( compactedItem->MAP(), str_value ) )
					compactedItem = compactedItem->MAP()->at ( str_value );
				string mapKey = *exp_item->MAP()->at ( *container )->STR();
				if ( !has ( mapObject->MAP(), mapKey ) )
					( *mapObject->MAP() ) [mapKey] = compactedItem;
				else {
					pobj& p = ( *mapObject->MAP() ) [mapKey];
					if ( !p->LIST() )
						p = mk_olist_obj ( olist ( 1, p ) );
					p->LIST()->push_back ( compactedItem );
				}
			}
			else {
				bool check = ( !compactArrays
				               || is ( container, { string ( str_set ), str_list } )
				               || is ( exp_prop, { str_list, str_graph } ) )
				             && ( !compactedItem->LIST() );
				if ( check )
					compactedItem = mk_olist_obj ( olist ( 1, compactedItem ) );
				if ( !has ( result, itemActiveProperty ) )
					( *result ) [itemActiveProperty] = compactedItem;
				else {
					pobj& p = ( *result ) [itemActiveProperty];
					if ( !p->LIST() )
						p = mk_olist_obj ( olist ( 1, p ) );
					add_all ( result->at ( itemActiveProperty )->LIST(),
					          compactedItem );
				}
			}
		}
	}
	return mk_somap_obj ( result );
}

bool jsonld_api::deepCompare ( pobj v1, pobj v2, bool listOrderMatters ) {
	if ( !v1 ) return !v2;
	if ( !v2 ) return !v1;
	if ( v1->MAP() && v2->MAP() ) {
		psomap m1 = v1->MAP(), m2 = v2->MAP();
		if ( m1->size() != m2->size() ) return false;
		for ( auto x : *m1 )
			if ( !has ( m2, x.first ) || !deepCompare ( x.second, m2->at ( x.first ), listOrderMatters ) )
				return false;
		return true;
	} else if ( v1->LIST() && v2->LIST() ) {
		polist l1 = v1->LIST(), l2 = v2->LIST();
		if ( l1->size() != l2->size() )
			return false;
		// used to mark members of l2 that we have already matched to avoid
		// matching the same item twice for lists that have duplicates
		bool *alreadyMatched = new bool[l2->size()];
		for ( size_t i = 0; i < l1->size(); ++i ) {
			pobj o1 = l1->at ( i );
			bool gotmatch = false;
			if ( listOrderMatters ) gotmatch = deepCompare ( o1, l2->at ( i ), listOrderMatters );
			else for ( size_t j = 0; j < l2->size(); j++ )
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

pobj jsonld_api::expand ( pcontext act_ctx, pstring act_prop, pobj element ) {
	pobj result;
	if ( !element ) result = 0;
	else if ( ! ( element->LIST() || element->MAP() ) )
		result = ( !act_prop || *act_prop == str_graph ) ? 0 : act_ctx->expandValue ( act_prop, element );
	else if ( element->LIST() ) {
		result = mk_olist_obj();
		for ( pobj item : *element->LIST() ) {
			pobj v = expand ( act_ctx, act_prop, item );
			if ( act_prop && ( ( *act_prop == str_list || ( act_ctx->getContainer ( act_prop ) && *act_ctx->getContainer ( act_prop ) == str_list ) ) )
			        && ( v->LIST() || ( v->MAP() && has ( v->MAP(), str_list ) ) ) ) throw Ex19;
			if ( v ) add_all ( result->LIST(), v );
		}
	} else if ( element->MAP() ) {
		psomap elem = element->MAP();
		if ( elem->find ( str_context ) != elem->end() ) act_ctx = act_ctx->parse ( elem->at ( str_context ) );
		result = mk_somap_obj();
		for ( auto x : *elem ) {
			string key = x.first;
			pobj value = x.second;
			if ( key == str_context ) continue;
			pstring exp_prop = act_ctx->expand_iri ( pstr ( key ), false, true, 0, 0 );
			pobj exp_val = 0;
			if ( !exp_prop || ( ( ( *exp_prop ) [0] != '?' /* vars support - out of spec */ &&  exp_prop->find ( ":" ) == string::npos ) && !keyword ( *exp_prop ) ) ) continue;
			if ( keyword ( *exp_prop ) ) {
				if ( act_prop && *act_prop == str_reverse ) throw Ex12;
				if ( has ( result, exp_prop ) ) throw std::runtime_error ( COLLIDING_KEYWORDS + tab + *exp_prop + string ( " already exists in result" ) );
				if ( *exp_prop == str_id ) {
					if ( !value->STR() ) throw Ex13;
					exp_val = make_shared <string_obj> ( act_ctx->expand_iri ( value->STR(), true, false, 0, 0 ) );
				} else if ( *exp_prop == str_type ) {
					if ( value->LIST() ) {
						exp_val = mk_olist_obj();
						for ( pobj v : *value->LIST() ) {
							if ( !v->STR() ) throw Ex14;
							exp_val->LIST()->push_back ( make_shared <string_obj> ( act_ctx->expand_iri ( v->STR(), true, true, 0, 0 ) ) );
						}
					} else if ( value->STR() )
						exp_val = make_shared <string_obj> ( act_ctx->expand_iri ( value->STR(), true, true, 0, 0 ) );
					else if ( value->MAP() ) {
						if ( value->MAP()->size() ) throw Ex15;
						exp_val = value;
					} else throw Ex16;
				} else if ( *exp_prop == str_graph ) exp_val = expand ( act_ctx, pstr ( str_graph ), value );
				else if ( *exp_prop == str_value ) {
					if ( value && ( value->MAP() || value->LIST() ) )
						throw std::runtime_error ( INVALID_VALUE_OBJECT_VALUE + tab + string ( "value of " ) + *exp_prop + string ( " must be a scalar or null" ) );
					if ( ! ( exp_val = value ) ) {
						( *result->MAP() ) [str_value] = 0;
						continue;
					}
				} else if ( *exp_prop == str_lang ) {
					if ( !value->STR() )
						throw std::runtime_error (
						    INVALID_LANGUAGE_TAGGED_STRING + tab
						    + string ( "Value of " ) + *exp_prop
						    + string ( " must be a string" ) );
					exp_val = make_shared <string_obj
					          > ( lower ( *value->STR() ) );
				} else if ( *exp_prop == str_index ) {
					if ( !value->STR() )
						throw std::runtime_error (
						    INVALID_INDEX_VALUE + tab
						    + string ( "Value of " ) + *exp_prop
						    + string ( " must be a string" ) );
					exp_val = value;
				} else if ( *exp_prop == str_list ) {
					if ( act_prop && *act_prop == str_graph )
						continue;
					exp_val = expand ( act_ctx, act_prop, value );
					if ( !exp_val->LIST() )
						exp_val = mk_olist_obj ( olist ( 1, exp_val ) );
					for ( auto o : *exp_val->LIST() )
						if ( o->MAP() && has ( o->MAP(), str_list ) ) throw Ex17;
				} else if ( *exp_prop == str_set )
					exp_val = expand ( act_ctx, act_prop, value );
				else if ( *exp_prop == str_reverse ) {
					if ( !value->MAP() ) throw Ex18;
					exp_val = expand ( act_ctx, pstr ( str_reverse ), value );
					if ( has ( exp_val->MAP(), str_reverse ) ) {
						psomap reverse =
						    exp_val->MAP()->at ( str_reverse )->MAP();
						for ( auto z : *reverse ) {
							string property = z.first;
							pobj item = z.second;
							if ( !has ( result, property ) )
								( *result->MAP() ) [property] = mk_olist_obj();
							add_all ( result->MAP()->at ( property )->LIST(),
							          item );
						}
					}
					if ( exp_val->MAP()->size()
					        > ( has ( exp_val->MAP(), str_reverse ) ? 1 : 0 ) ) {
						if ( !has ( result->MAP(), str_reverse ) )
							( *result->MAP() ) [str_reverse] = mk_somap_obj();
						psomap reverseMap =
						    result->MAP()->at ( str_reverse )->MAP();
						for ( auto t : *exp_val->MAP() ) {
							string property = t.first;
							if ( property == str_reverse )
								continue;
							polist items =
							    exp_val->MAP()->at ( property )->LIST();
							for ( pobj item : *items ) {
								if ( has ( item->MAP(), str_value )
								        || has ( item->MAP(), str_list ) )
									throw std::runtime_error (
									    INVALID_REVERSE_PROPERTY_VALUE );
								if ( !has ( reverseMap, property ) )
									( *reverseMap ) [property] =
									    mk_olist_obj();
								reverseMap->at ( property )->LIST()->push_back (
								    item );
							}
						}
					}
					continue;
				} else if ( is ( exp_prop, { str_explicit, str_default,
				                             str_embed, str_embedChildren, str_omitDefault
				                           } ) )
					exp_val = expand ( act_ctx, exp_prop, value );
				if ( exp_val )
					( *result->MAP() ) [*exp_prop] = exp_val;
				continue;
			} else if ( act_ctx->getContainer ( key )
			            && *act_ctx->getContainer ( key ) == str_lang
			            && value->MAP() ) {
				exp_val = mk_olist_obj();
				for ( auto yy : *value->MAP() ) {
					string language = yy.first;
					pobj languageValue = yy.second;
					if ( !languageValue->LIST() )
						languageValue = mk_olist_obj (
						                    olist ( 1, languageValue ) );
					for ( pobj item : *languageValue->LIST() ) {
						if ( !item->STR() )
							throw std::runtime_error (
							    INVALID_LANGUAGE_MAP_VALUE + tab
							    + string ( "Expected " )
							    + item->toString()
							    + string ( " to be a string" ) );
						somap tmp;
						tmp[str_value] = item;
						tmp[str_lang] = make_shared <string_obj
						                > ( lower ( language ) );
						exp_val->LIST()->push_back ( mk_somap_obj ( tmp ) );
					}
				}
			} else if ( act_ctx->getContainer ( key )
			            && *act_ctx->getContainer ( key ) == str_index
			            && value->MAP() ) {
				exp_val = mk_olist_obj();
				for ( auto xx : *value->MAP() ) {
					pobj indexValue = xx.second;
					if ( !indexValue->LIST() )
						indexValue = mk_olist_obj ( olist ( 1, indexValue ) );
					indexValue = expand ( act_ctx, pstr ( key ), indexValue );
					for ( pobj item : *indexValue->LIST() ) {
						if ( !has ( item->MAP(), str_index ) )
							( *item->MAP() ) [str_index] = make_shared
							                               <string_obj> ( xx.first );
						exp_val->LIST()->push_back ( item );
					}
				}
			} else
				exp_val = expand ( act_ctx, pstr ( key ), value );
			if ( !exp_val )
				continue;
			if ( act_ctx->getContainer ( key )
			        && *act_ctx->getContainer ( key ) == str_list
			        && !has ( exp_val->MAP(), str_list ) ) {
				auto tmp = exp_val;
				if ( !tmp->LIST() )
					tmp = mk_olist_obj ( olist ( 1, tmp ) );
				exp_val = mk_somap_obj();
				( *exp_val->MAP() ) [str_list] = tmp;
			} else {
				if ( act_ctx->isReverseProperty ( key ) ) {
					if ( !hasreverse ( result ) )
						( *result->MAP() ) [str_reverse] = mk_somap_obj();
					psomap reverseMap = getreverse ( result )->MAP();
					if ( !exp_val->LIST() )
						exp_val = mk_olist_obj ( olist ( 1, exp_val ) );
					for ( pobj item : *exp_val->LIST() ) {
						if ( hasvalue ( item->MAP() ) && haslist ( item->MAP() ) )
							throw INVALID_REVERSE_PROPERTY_VALUE;
						if ( !has ( reverseMap, exp_prop ) )
							( *reverseMap ) [*exp_prop] = mk_olist_obj();
						add_all ( reverseMap->at ( *exp_prop )->LIST(), item );
					}
				} else {
					if ( !has ( result, exp_prop ) )
						( *result->MAP() ) [*exp_prop] = mk_olist_obj();
					add_all ( result->MAP()->at ( *exp_prop )->LIST(), exp_val );
				}
			}
		}
		if ( hasvalue ( result ) ) {
			somap ks = *result->MAP();
			if ( hasvalue ( ks ) )
				ks.erase ( str_value );
			if ( hasindex ( ks ) )
				ks.erase ( str_index );
			bool langremoved = haslang ( ks ), typeremoved = hastype ( ks );
			if ( langremoved )
				ks.erase ( str_lang );
			if ( typeremoved )
				ks.erase ( str_type );
			if ( ( langremoved && typeremoved ) || ks.size() )
				throw std::runtime_error (
				    INVALID_VALUE_OBJECT + tab
				    + string ( "value object has unknown keys" ) );
			pobj rval = getvalue ( result );
			if ( !rval )
				result = 0;
			else if ( !rval->STR() && haslang ( result ) )
				throw std::runtime_error (
				    INVALID_LANGUAGE_TAGGED_VALUE + tab
				    + string (
				        "when @language is used, @value must be a string" ) );
			else if ( hastype ( result ) )
				if ( ! ( gettype ( result )->STR() )
				        || startsWith ( *gettype ( result )->STR(), "_:" )
				        || gettype ( result )->STR()->find ( ":" )
				        == string::npos )
					throw std::runtime_error (
					    INVALID_TYPED_VALUE + tab
					    + string (
					        "value of @type must be an IRI" ) );
		} else if ( hastype ( result ) ) {
			pobj rtype = gettype ( result );
			if ( !rtype->LIST() )
				( *result->MAP() ) [str_type] = mk_olist_obj ( olist ( 1, rtype ) );
		} else if ( hasset ( result ) || haslist ( result ) ) {
			if ( result->MAP()->size() > ( hasindex ( result ) ? 2 : 1 ) )
				throw std::runtime_error (
				    INVALID_SET_OR_LIST_OBJECT + tab
				    + string (
				        "@set or @list may only contain @index" ) );
			if ( hasset ( result ) )
				result = getset ( result );
		}
		if ( haslang ( result ) && result->MAP()->size() == 1 )
			result = 0;
		if ( !act_prop || *act_prop == str_graph ) {
			if ( result
			        && ( ( !result->MAP()->size() || hasvalue ( result )
			               || haslist ( result ) ) ) )
				result = 0;
			else if ( result && hasid ( result ) && result->MAP()->size() == 1 )
				result = 0;
		}
	}
	if ( result && result->MAP() && result->MAP()->size() == 1
	        && hasgraph ( result ) )
		result = getgraph ( result );
	if ( !result )
		result = mk_olist_obj();
	if ( !result->LIST() )
		result = mk_olist_obj ( olist ( 1, result ) );
	return result;
}

void jsonld_api::gen_node_map ( pobj element, psomap nodeMap, string activeGraph, pobj activeSubject, pstring act_prop, psomap list ) {
	if ( !element ) return;
	if ( element->LIST() ) {
		for ( pobj item : *element->LIST() ) gen_node_map ( item, nodeMap, activeGraph, activeSubject, act_prop, list );
		return;
	}
	psomap elem = element->MAP();
	if ( !has ( nodeMap, activeGraph ) ) ( *nodeMap ) [activeGraph] = mk_somap_obj();
	psomap graph = nodeMap->at ( activeGraph )->MAP(), node = ( activeSubject && activeSubject->STR() ) ?  graph->at ( *activeSubject->STR() )->MAP() : 0;
	if ( hastype ( elem ) ) {
		vector<string> oldTypes, newTypes;
		if ( gettype ( elem )->LIST() ) oldTypes = vec2vec ( gettype ( elem )->LIST() );
		else {
			oldTypes = vector<string>();	//mk_olist_obj();
			oldTypes.push_back ( *elem->at ( str_type )->STR() );
		}
		for ( string item : oldTypes ) {
			if ( startsWith ( item, "_:" ) ) newTypes.push_back ( gen_bnode_id ( item ) );
			else newTypes.push_back ( item );
		}
		( *elem ) [str_type] = gettype ( elem )->LIST() ? ( pobj ) mk_olist_obj ( vec2vec ( newTypes ) ) : make_shared <string_obj> ( newTypes[0] );
	}
	if ( hasvalue ( elem ) ) {
		if ( !list ) mergeValue ( node, act_prop, element );
		else mergeValue ( list, str_list, element );
	} else if ( haslist ( elem ) ) {
		psomap result = make_shared<somap>();
		( *result ) [str_list] = mk_olist_obj();
		gen_node_map ( getlist ( elem ), nodeMap, activeGraph, activeSubject, act_prop, result );
		mergeValue ( node, act_prop, mk_somap_obj ( result ) );
	} else {
		string id;
		if ( hasid ( elem ) && elem->at ( str_id ) && elem->at ( str_id )->STR() ) {
			id = *elem->at ( str_id )->STR();
			elem->erase ( str_id );
			if ( startsWith ( id, "_:" ) ) id = gen_bnode_id ( id );
		} else id = gen_bnode_id();
		if ( !has ( graph, id ) ) {
			somap tmp;
			tmp[str_id] = make_shared <string_obj> ( id );
			( *graph ) [id] = mk_somap_obj ( tmp );
		}
		if ( activeSubject && activeSubject->MAP() ) mergeValue ( get ( graph, id )->MAP(), act_prop, activeSubject );
		else if ( act_prop ) {
			somap ref;
			ref[str_id] = make_shared <string_obj> ( id );
			if ( !list ) mergeValue ( node, act_prop, mk_somap_obj ( ref ) );
			else mergeValue ( list, str_list, mk_somap_obj ( ref ) );
		}
		node = graph->at ( id )->MAP();
		if ( hastype ( elem ) ) {
			if ( gettype ( elem )->LIST() )
				for ( pobj type : *gettype ( elem )->LIST() )
					if ( type ) mergeValue ( node, str_type, type );
			elem->erase ( str_type );
		}
		if ( hasindex ( elem ) ) {
			pobj elemIndex = getindex ( elem );
			elem->erase ( str_index );
			if ( hasindex ( node ) ) {
				if ( !deepCompare ( getindex ( node ), elemIndex ) ) throw CONFLICTING_INDEXES;
			} else ( *node ) [str_index] = elemIndex;
		}
		if ( hasreverse ( elem ) ) {
			psomap refnode = make_shared<somap>(), revmap = elem->at ( str_reverse )->MAP();
			( *refnode ) [str_id] = make_shared <string_obj> ( id );
			elem->erase ( str_reverse );
			if ( revmap )
				for ( auto x : *revmap ) {
					string prop = x.first;
					polist values = revmap->at ( prop )->LIST();
					if ( values ) for ( pobj value : *values )
							gen_node_map ( value, nodeMap, activeGraph, mk_somap_obj ( refnode ), make_shared <string> ( prop ), 0 );
				}
		}
		if ( hasgraph ( elem ) ) {
			gen_node_map ( getgraph ( elem ), nodeMap, id, 0, 0, 0 );
			elem->erase ( str_graph );
		}
		//			final List<String> keys = new ArrayList<String> ( elem.keySet() );
		//			Collections.sort ( keys );
		if ( elem )
			for ( auto z : *elem ) {
				string property = z.first;
				pobj value = z.second;
				if ( startsWith ( property, "_:" ) ) property = gen_bnode_id ( property );
				if ( !has ( node, property ) ) ( *node ) [property] = mk_olist_obj();
				gen_node_map ( value, nodeMap, activeGraph, make_shared <string_obj> ( id ), make_shared <string> ( property ), 0 );
			}
	}
}
prdf_db jsonld_api::toRDF() {
	psomap nodeMap = make_shared<somap>();
	( *nodeMap ) [str_default] = mk_somap_obj();
	gen_node_map ( value, nodeMap );
	rdf_db r ( *this );
	for ( auto g : *nodeMap ) {
		if ( is_rel_iri ( g.first ) )
			continue;
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	return make_shared <rdf_db> ( r );
}

prdf_db jsonld_api::toRDF ( pobj input, jsonld_options options ) {
	pobj expandedInput = jsonld::expand ( input, options );
	jsonld_api api ( expandedInput, options );
	prdf_db dataset = api.toRDF();

	// generate namespaces from context
	if ( options.useNamespaces ) {
		polist _input;
		if ( expandedInput->LIST() )
			_input = expandedInput->LIST();
		else {
			_input = mk_olist();
			_input->push_back ( expandedInput );
		}
		for ( auto e : *_input )
			if ( hascontext ( e ) )
				dataset->parse_ctx ( getcontext ( e ) );

	}
	return dataset;
}

pobj expand ( pobj input, jsonld_options opts ) {
	if ( !input ) return 0;
	if ( input->STR() && input->STR()->find ( ":" ) != string::npos ) {
		input = load ( *input->STR() ).document;
		if ( !opts.base )
			opts.base = pstr ( *input->STR() );
	}
	std::shared_ptr<context_t> act_ctx = make_shared <context_t> ( opts );
	if ( opts.expandContext ) {
		if ( opts.expandContext->MAP()
		        && has ( opts.expandContext->MAP(), str_context ) )
			opts.expandContext = opts.expandContext->MAP()->at ( str_context );
		act_ctx = act_ctx->parse ( opts.expandContext );
	}
	auto expanded = jsonld_api ( opts ).expand ( act_ctx, 0, input );
	if ( hasgraph ( expanded ) )
		expanded = getgraph ( expanded );
	else if ( !expanded )
		expanded = mk_olist_obj();
	if ( !expanded->LIST() )
		expanded = mk_olist_obj ( olist ( 1, expanded ) );
	return expanded;
}

size_t write_data ( void *ptr, size_t size, size_t n, void *stream ) {
	string data ( ( const char* ) ptr, ( size_t ) size * n );
	* ( ( std::stringstream* ) stream ) << data << std::endl;
	return size * n;
}

string download ( const string& url ) {
	static const string ACCEPT_HEADER =
	    "application/ld+json, application/json;q=0.9, application/javascript;q=0.5, text/javascript;q=0.5, text/plain;q=0.2, */*;q=0.1";
	struct curl_slist *headers = 0;
	headers = curl_slist_append ( headers, "Content-Type: text/xml" );
	curl_easy_setopt ( curl, CURLOPT_HTTPHEADER, headers );
	curl_easy_setopt ( curl, CURLOPT_URL, url.c_str() );
	curl_easy_setopt ( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt ( curl, CURLOPT_NOSIGNAL, 1 );
	curl_easy_setopt ( curl, CURLOPT_ACCEPT_ENCODING, "deflate" );
	std::stringstream out;
	curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, write_data );
	curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &out );
	CURLcode res = curl_easy_perform ( curl );
	if ( res != CURLE_OK )
		throw std::runtime_error (
		    string ( "curl_easy_perform() failed: " )
		    + curl_easy_strerror ( res ) );
	string r = out.str();
	cout << "downloaded file: " << r << endl;
	return r;
}

pobj fromURL ( const string& url ) {
	json_spirit::mValue r;
	json_spirit::read_string ( download ( url ), r );
	return convert ( r );
}

remote_doc_t load ( const string& url ) {
	pobj p;
	remote_doc_t doc ( url, p );
	try {
		doc.document = fromURL ( url );
	} catch ( ... ) {
		throw std::runtime_error ( LOADING_REMOTE_CONTEXT_FAILED + tab + url );
	}
	return doc;
}

json_spirit::mValue convert ( obj& v ) {
	typedef json_spirit::mValue val;
	val r;
	if ( v.UINT() ) return val ( *v.UINT() );
	if ( v.INT() )  return val ( *v.INT() );
	if ( v.STR() )  return val ( *v.STR() );
	if ( v.DOUBLE() )  return val ( *v.DOUBLE() );
	if ( v.BOOL() )  return val ( *v.BOOL() );
	else if ( v.LIST() ) {
		val::Array a;
		for ( auto x : *v.LIST() ) a.push_back ( convert ( *x ) );
		return val ( a );
	} else {
		if ( !v.MAP() ) throw "logic error";
		val::Object a;
		for ( auto x : *v.MAP() ) a[x.first] = convert ( *x.second );
		return val ( a );
	}
}

json_spirit::mValue convert ( pobj v ) {
	return convert ( *v );
}

pobj convert ( const json_spirit::mValue& v ) {
	using namespace std;
	pobj r;
	if ( v.is_uint64() ) r = make_shared<uint64_t_obj> ( v.get_uint64() );
	else if ( v.isInt() ) r = make_shared<int64_t_obj> ( v.get_int64() );
	else if ( v.isString() ) r = make_shared<string_obj> ( v.get_str() );
	else if ( v.isDouble() ) r = make_shared<double_obj> ( v.get_real() );
	else if ( v.isBoolean() ) r = make_shared<bool_obj> ( v.get_bool() );
	else if ( v.is_null() ) r = 0;
	else if ( v.isList() ) {
		r = make_shared<olist_obj>();
		auto a = v.get_array();
		for ( auto x : a ) r->LIST()->push_back ( convert ( x ) );
	} else {
		if ( !v.isMap() ) throw "logic error";
		r = make_shared<somap_obj>();
		auto a = v.get_obj();
		for ( auto x : a ) ( *r->MAP() ) [x.first] = convert ( x.second );
	}
	return r;
}

size_t jsonld_api::blankNodeCounter = 0;
map<string, string> jsonld_api::bnode_id_map;
void* curl = curl_easy_init();

}

string obj::toString() {
	return json_spirit::write_string ( jsonld::convert ( *this ), json_spirit::pretty_print );
}

