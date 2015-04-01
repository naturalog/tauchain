// package com.github.jsonldjava.impl;

// import static com.github.jsonldjava.core.RDFDatasetUtils.parseNQuads;

// import com.github.jsonldjava.core.JsonLdError;
// import com.github.jsonldjava.core.RDFDataset;
// import com.github.jsonldjava.core.RDFParser;

class NQuadRDFParser : public RDFParser {
	virtual
public: RDFDataset parse ( Object input )  {
		if ( input.isString )
			return parseNQuads ( ( String ) input ); else {
			throw new JsonLdError ( INVALID_INPUT,
			                        "NQuad Parser expected string input." );
		}
	}

}
;
