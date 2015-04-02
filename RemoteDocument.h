#ifndef __REMOTE__DOCUMENT_H__
#define __REMOTE__DOCUMENT_H__

#include "defs.h"

template<typename Object>
struct RemoteDocument {
	String getDocumentUrl() {
		return documentUrl;
	}

	void setDocumentUrl ( String documentUrl_ ) {
		documentUrl = documentUrl_;
	}

	const Object& getDocument() {
		return document;
	}

	void setDocument ( const Object& document_ );

	String getContextUrl() {
		return contextUrl;
	}

	void setContextUrl ( String contextUrl_ ) {
		contextUrl = contextUrl_;
	}

	String documentUrl;
	Object& document;
	String contextUrl;

	RemoteDocument ( String url, const Object& document_ /*= null*/, String context = "" );
};
#endif
