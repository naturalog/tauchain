#ifndef __REMOTE__DOCUMENT_H__
#define __REMOTE__DOCUMENT_H__

#include "defs.h"

template<typename Object>
struct RemoteDocument {
	void setDocument ( const Object& document_ ) {
		document = document_;
	}

	RemoteDocument ( String url, const Object& document_ /*= null*/, String context = "" ) :
		documentUrl ( url ),
		document ( document_ ),
		contextUrl ( context ) {
	}
	String getDocumentUrl() {
		return documentUrl;
	}

	void setDocumentUrl ( String documentUrl_ ) {
		documentUrl = documentUrl_;
	}

	const Object& getDocument() {
		return document;
	}

	String getContextUrl() {
		return contextUrl;
	}

	void setContextUrl ( String contextUrl_ ) {
		contextUrl = contextUrl_;
	}

	String documentUrl;
	Object& document;
	String contextUrl;

};
#endif
