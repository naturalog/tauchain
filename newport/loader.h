
struct remote_doc_t {
	string url;
	pstring context_url;
	pobj document;
	remote_doc_t ( string url_, const pobj& document_ = 0, pstring context = 0 ) : url ( url_ ), document ( document_ ), context_url ( context ) { }
};

void* curl = curl_easy_init();
	pobj convert ( const json_spirit::mValue& v ) {
		if ( v.is_uint64() ) return make_shared<uint64_t_obj> ( v.get_uint64() );
		if ( v.isInt() ) make_shared<int64_t_obj> ( v.get_int64() );
		if ( v.isString() ) return make_shared<string_obj> ( v.get_str() );
		if ( v.isDouble() ) return make_shared<double_obj> ( v.get_real() );
		if ( v.isBoolean() ) return make_shared<bool_obj> ( v.get_bool() );
		if ( v.is_null() ) return 0; //make_shared<null_obj>();
		pobj r;
		if ( v.isList() ) {
			r = make_shared<olist_obj>();
			auto a = v.get_array();
			for ( auto x : a ) r->LIST()->push_back ( convert ( x ) );
			return r;
		}
		if ( !v.isMap() ) throw "logic error";
		r = make_shared<somap_obj>();
		auto a = v.get_obj();
		for ( auto x : a ) ( *r->MAP() ) [x.first] = convert ( x.second );
		return r;
	}

size_t write_data ( void *ptr, size_t size, size_t n, void *stream ) {
		string data ( ( const char* ) ptr, ( size_t ) size * n );
		* ( ( std::stringstream* ) stream ) << data << std::endl;
		return size * n;
	}

	string download ( const string& url ) {
		curl_easy_setopt ( curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt ( curl, CURLOPT_FOLLOWLOCATION, 1L );
		curl_easy_setopt ( curl, CURLOPT_NOSIGNAL, 1 );
		curl_easy_setopt ( curl, CURLOPT_ACCEPT_ENCODING, "deflate" );
		std::stringstream out;
		curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, write_data );
		curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &out );
		CURLcode res = curl_easy_perform ( curl );
		if ( res != CURLE_OK ) throw std::runtime_error ( "curl_easy_perform() failed: "s + curl_easy_strerror ( res ) );
		return out.str();
	}

	pobj fromURL ( const string& url ) {
		json_spirit::mValue r;
		json_spirit::read ( download ( url ), r );
		return convert ( r );
	}

	remote_doc_t load ( const string& url )  {
		pobj p;
		remote_doc_t doc ( url, p );
		try {
			doc.document = fromURL ( url );
		} catch ( ... ) {
			throw LOADING_REMOTE_CONTEXT_FAILED + "\t" + url;
		}
		return doc;
	}

