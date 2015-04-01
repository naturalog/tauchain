// package com.github.jsonldjava.impl;

// import com.github.jsonldjava.core.JsonLdTripleCallback;
// import com.github.jsonldjava.core.RDFDataset;
// import com.github.jsonldjava.core.RDFDatasetUtils;

class NQuadTripleCallback : public JsonLdTripleCallback {
	virtual
public: Object call ( RDFDataset dataset ) {
		return RDFDatasetUtils.toNQuads ( dataset );
	}
}
;
