// package com.github.jsonldjava.core;
#ifndef __JSONLD_OPTIONS_H__
#define __JSONLD_OPTIONS_H__

#include "DocumentLoader.h"

class JsonLdOptions {
	String base;// = null;
	boolean produceGeneralizedRdf = false;
	String processingMode = "json-ld-1.0";
	DocumentLoader documentLoader;// = new DocumentLoader();
	Boolean embed = bnull;
	Boolean Explicit = bnull;
	Boolean omitDefault = bnull;
	Boolean useRdfType = false;
	Boolean useNativeTypes = false;
public:
	JsonLdOptions ( String base = "" );
	Boolean compactArrays = true;
	const Object& expandContext; 
	Boolean getEmbed() { return embed; } 
	void setEmbed ( Boolean embed_ ) { embed = embed_; } 
	Boolean getExplicit() { return Explicit; } 
	void setExplicit ( Boolean Explicit_ ) { Explicit = Explicit_; } 
	Boolean getOmitDefault() { return omitDefault; } 
	void setOmitDefault ( Boolean omitDefault_ ) { omitDefault = omitDefault_; } 
	Boolean getCompactArrays() { return compactArrays; } 
	void setCompactArrays ( Boolean compactArrays_ ) { compactArrays = compactArrays_; } 
	const Object& getExpandContext() { return expandContext; } 
	void setExpandContext ( const Object& expandContext_ );
	String getProcessingMode() { return processingMode; } 
	void setProcessingMode ( String processingMode_ ) { processingMode = processingMode_; } 
	String getBase() { return base; } 
	void setBase ( String base_ ) { base = base_; } 
	Boolean getUseRdfType() { return useRdfType; } 
	void setUseRdfType ( Boolean useRdfType_ ) { useRdfType = useRdfType_; } 
	Boolean getUseNativeTypes() { return useNativeTypes; } 
	void setUseNativeTypes ( Boolean useNativeTypes_ ) { useNativeTypes = useNativeTypes_; } 
	boolean getProduceGeneralizedRdf() { return produceGeneralizedRdf; } 
	void setProduceGeneralizedRdf ( Boolean produceGeneralizedRdf_ ) { produceGeneralizedRdf = produceGeneralizedRdf_; } 
	DocumentLoader getDocumentLoader() { return documentLoader; } 
	void setDocumentLoader ( DocumentLoader documentLoader_ ) { documentLoader = documentLoader; } 
	// TODO: THE FOLLOWING ONLY EXIST SO I DON'T HAVE TO DELETE A LOT OF CODE,
	// REMOVE IT WHEN DONE
	String format = "";//null;
	Boolean useNamespaces = false;
	String outputForm = "";//null;
};
#endif
