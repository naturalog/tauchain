// package com.github.jsonldjava.core;

struct RemoteDocument {
	String getDocumentUrl() {
		return documentUrl;
	}

	void setDocumentUrl ( String documentUrl_ ) {
		documentUrl = documentUrl_;
	}

	Object getDocument() {
		return document;
	}

	void setDocument ( Object document_ ) {
		document = document_;
	}

	String getContextUrl() {
		return contextUrl;
	}

	void setContextUrl ( String contextUrl_ ) {
		contextUrl = contextUrl_;
	}

	String documentUrl;
	Object document;
	String contextUrl;

	RemoteDocument ( String url, Object document_ = null, String context = "" ) :
		documentUrl ( url ),
		document ( document_ ),
		contextUrl ( context ) {
	}
};
