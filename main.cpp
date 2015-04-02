#include "defs.h"

#include "json_spirit_reader.h"
#include "json_spirit_writer.h"
//pedef double Double;
//typedef boost::variant<std::string, null> String;
//typedef boost::variant<bool, null> boolean;
//typedef boost::variant<double, null> Double;

typedef json_spirit::mValue Object;
#include "Obj.h"
#include "JsonLdError.h"
#include "RemoteDocument.h"
#include "DocumentLoader.h"
#include "JsonLdOptions.h"
//#include "Regex.h"
#include "JsonLdUtils.h"
#include "JsonLdConsts.h"
#include "JsonLdProcessor.h"
//#include "NQuadTripleCallback.h"
//#include "RDFParser.h"
//#include "NQuadRDFParser.h"
#include "JsonLdTripleCallback.h"
//#include "RDFDatasetUtils.h"
//#include "TurtleTripleCallback.h"
#include "UniqueNamer.h"
//#include "TurtleRDFParser.h"
#include "JarCacheStorage.h"
#include "RDFDataset.h"
#include "NormalizeUtils.h"
#include "JsonLdUrl.h"
#include "JarCacheResource.h"
#include "JsonLdApi.h"
#include "Context.cpp"
void RemoteDocument::setDocument ( const Object& document_ ) {
	document = document_;
}
RRemoteDocument:: emoteDocument ( String url, const Object& document_ /*= null*/, String context ) :
	documentUrl ( url ),
	document ( document_ ),
	contextUrl ( context ) {
}
RemoteDocument DocumentLoader::loadDocument ( String url )  {
	RemoteDocument doc/* = new RemoteDocument*/ ( url, Object() );
	try {
		doc.setDocument ( fromURL ( /*new URL*/ ( url ) ) );
	} catch ( ... ) {
		throw JsonLdError ( LOADING_REMOTE_CONTEXT_FAILED, url );
	}
	return doc;
}

const Object& DocumentLoader::fromURL ( String url ) {
	Object r;
	json_spirit::read ( download ( url ), r );
	return r;
}
JsonLdOptions::JsonLdOptions ( String base = "" ) : expandContext ( null ) {
	setBase ( base );
}
void JsonLdOptions::setExpandContext ( const Object& expandContext_ ) {
	expandContext = expandContext_;
}
