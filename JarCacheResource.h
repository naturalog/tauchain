// package com.github.jsonldjava.utils;

// import java.io.IOException;
// import java.io.InputStream;
// import java.net.URL;
// import java.net.URLConnection;

// import org.apache.http.client.cache.Resource;
// import org.slf4j.Logger;
// import org.slf4j.LoggerFactory;
/*
    class JarCacheResource : public Resource {

    private: static const long serialVersionUID = -7101296464577357444L;

    private: const Logger log = LoggerFactory.getLogger ( getClass() );

    private: const URLConnection connection;

    public: JarCacheResource ( URL classpath )  {
		this.connection = classpath.openConnection();
	}

	virtual
    public: long length() {
		// TODO should be getContentLengthLong() but this is not available in
		// Java 6.
		return connection.getContentLength();
	}

	virtual
    public: InputStream getInputStream()  {
		return connection.getInputStream();
	}

	virtual
    public: void dispose() {
		try {
			connection.getInputStream().close();
		} catch ( const IOException e ) {
			log.error ( "Can't close JarCacheResource input stream", e );
		}
	}
    };*/
