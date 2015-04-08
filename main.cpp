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


//const Node& RDFDataset::first = IRI ( RDF_FIRST );
//const Node& RDFDataset::rest = IRI ( RDF_REST );
//const Node& RDFDataset::nil = IRI ( RDF_NIL );
