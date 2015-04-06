/*
    #include "HttpCacheStorage.h"
    class JarCacheStorage : public HttpCacheStorage {

    private:
    static const String JARCACHE_JSON = "jarcache.json";

    const Logger log = LoggerFactory.getLogger ( getClass() );

    const CacheConfig cacheConfig = new CacheConfig();
    ClassLoader classLoader;

    public:
    ClassLoader getClassLoader() {
		if ( classLoader != null )
			return classLoader;
		return Thread.currentThread().getContextClassLoader();
	}

    void setClassLoader ( ClassLoader classLoader ) {
		this.classLoader = classLoader;
	}

    JarCacheStorage() {
		this ( null );
	}

    JarCacheStorage ( ClassLoader classLoader ) {
		setClassLoader ( classLoader );
		cacheConfig.setMaxObjectSize ( 0 );
		cacheConfig.setMaxCacheEntries ( 0 );
		cacheConfig.setMaxUpdateRetries ( 0 );
		cacheConfig.getMaxCacheEntries();
	}

	virtual
    void putEntry ( String key, HttpCacheEntry entry )  {
		// ignored

	}

	ObjectMapper mapper = new ObjectMapper();

	virtual
    HttpCacheEntry getEntry ( String key )  {
		log.trace ( "Requesting " + key );
		URI requestedUri;
		try {
			requestedUri = new URI ( key );
		} catch ( const URISyntaxException e ) {
			return null;
		}
		if ( ( requestedUri.getScheme().equals ( "http" ) && requestedUri.getPort() == 80 )
		        || ( requestedUri.getScheme().equals ( "https" ) && requestedUri.getPort() == 443 ) ) {
			// Strip away default http ports
			try {
				requestedUri = new URI ( requestedUri.getScheme(), requestedUri.getHost(),
				                         requestedUri.getPath(), requestedUri.getFragment() );
			} catch ( const URISyntaxException e ) {
			}
		}

		const Enumeration<URL> jarcaches = getResources();
		while ( jarcaches.hasMoreElements() ) {
			const URL url = jarcaches.nextElement();

			const JsonNode tree = getJarCache ( url );
			// TODO: Cache tree per URL
			for ( const JsonNode node : tree ) {
				const URI uri = URI.create ( node.get ( "Content-Location" ).asText() );
				if ( uri.equals ( requestedUri ) )
					return cacheEntry ( requestedUri, url, node );

			}
		}
		return null;
	}

    private:
    Enumeration<URL> getResources()  {
		const ClassLoader cl = getClassLoader();
		if ( cl != null )
			return cl.getResources ( JARCACHE_JSON ); else
			return ClassLoader.getSystemResources ( JARCACHE_JSON );
	}

    protected:
    ConcurrentMap<URI, SoftReference<JsonNode>> jarCaches = new ConcurrentHashMap<URI, SoftReference<JsonNode>>();

    JsonNode getJarCache ( URL url ) , JsonProcessingException {

		URI uri;
		try {
			uri = url.toURI();
		} catch ( const URISyntaxException e ) {
			throw new IllegalArgumentException ( "Invalid jarCache URI " + url, e );
		}

		// Check if we have one from before - we'll use SoftReference so that
		// the maps reference is not counted for garbage collection purposes
		const SoftReference<JsonNode> jarCacheRef = jarCaches.get ( uri );
		if ( jarCacheRef != null ) {
			const JsonNode jarCache = jarCacheRef.get();
			if ( jarCache != null )
				return jarCache; else
				jarCaches.remove ( uri );
		}

		// Only parse again if the optimistic get failed
		const JsonNode tree = mapper.readTree ( url );
		// Use putIfAbsent to ensure concurrent reads do not return different
		// JsonNode objects, for memory management purposes
		const SoftReference<JsonNode> putIfAbsent = jarCaches.putIfAbsent ( uri,
		new SoftReference<JsonNode> ( tree ) );
		if ( putIfAbsent != null ) {
			const JsonNode returnValue = putIfAbsent.get();
			if ( returnValue != null )
				return returnValue; else {
				// Force update the reference if the existing reference had
				// been garbage collected
				jarCaches.put ( uri, new SoftReference<JsonNode> ( tree ) );
			}
		}
		return tree;
	}

    HttpCacheEntry cacheEntry ( URI requestedUri, URL baseURL, JsonNode cacheNode )
	throws MalformedURLException, IOException {
		const URL classpath = new URL ( baseURL, cacheNode.get ( "X-Classpath" ).asText() );
		log.debug ( "Cache hit for " + requestedUri );
		log.trace ( "{}", cacheNode );

		const List<Header> responseHeaders = new ArrayList<Header>();
		if ( !cacheNode.has ( HTTP.DATE_HEADER ) ) {
			responseHeaders
			.add ( new BasicHeader ( HTTP.DATE_HEADER, DateUtils.formatDate ( new Date() ) ) );
		}
		if ( !cacheNode.has ( HeaderConstants.CACHE_CONTROL ) ) {
			responseHeaders.add ( new BasicHeader ( HeaderConstants.CACHE_CONTROL,
			HeaderConstants.CACHE_CONTROL_MAX_AGE + "=" + Integer.MAX_VALUE ) );
		}
		const Resource resource = new JarCacheResource ( classpath );
		const Iterator<String> fieldNames = cacheNode.fieldNames();
		while ( fieldNames.hasNext() ) {
			const String headerName = fieldNames.next();
			const JsonNode header = cacheNode.get ( headerName );
			// TODO: Support multiple headers with []
			responseHeaders.add ( new BasicHeader ( headerName, header.asText() ) );
		}

		return new HttpCacheEntry ( new Date(), new Date(), new BasicStatusLine ( HttpVersion.HTTP_1_1,
		200, "OK" ), responseHeaders.toArray ( new Header[0] ), resource );
	}
    public:
	virtual
    void removeEntry ( String key )  {
		// Ignored
	}

	virtual
    void updateEntry ( String key, HttpCacheUpdateCallback callback ) ,
	HttpCacheUpdateException {
		// ignored
	}

    CacheConfig getCacheConfig() {
		return cacheConfig;
	}

    };
*/
