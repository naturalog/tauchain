#ifndef __OBJ_H__
#define __OBJ_H__

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <stdio.h>
#include <signal.h>
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <ostream>

//#include "logger.h"
extern bool deref, shorten;
//#define DEBUG
//#define VERBOSE
#ifdef DEBUG
//logger _logger;
extern bool autobt, _pause;
void bt();
void dopause();
#define trace(x) std::clog<<__FILE__<<':'<<__LINE__<<'\t'<<x; if (_pause) dopause()
#else
void bt();
#define trace(x)
#endif

typedef std::nullptr_t null;
typedef std::string string;
typedef std::shared_ptr<string> pstring;
typedef std::shared_ptr<bool> pbool;

pstring pstr ( const string& s );
pstring pstr ( const char* s );
pstring pstr ( const unsigned char* s );

extern std::ostream& dout;

class obj {
protected:
	obj() {}
public:
	typedef std::shared_ptr<obj> pobj;
	typedef std::map<string, pobj> somap;
	typedef std::vector<pobj> olist;
	typedef std::shared_ptr<somap> psomap;

	virtual std::shared_ptr<uint64_t> UINT() {
		return 0;
	}
	virtual std::shared_ptr<int64_t> INT() {
		return 0;
	}
	virtual std::shared_ptr<bool> BOOL() {
		return 0;
	}
	virtual std::shared_ptr<string> STR() {
		return 0;
	}
	virtual std::shared_ptr<somap> MAP() {
		return 0;
	}
	virtual std::shared_ptr<olist> LIST() {
		return 0;
	}
	virtual std::shared_ptr<double> DOUBLE() {
		return 0;
	}
	virtual std::shared_ptr<null> Null() {
		return 0;
	}
	size_t size();
	bool STR ( const string& x );
	std::shared_ptr<obj> MAP ( const string& k );
	bool map_and_has ( const string& k );
	bool map_and_has_null ( const string& k );
	virtual string type_str() const = 0;
	virtual bool equals ( const obj& o ) const = 0;
	virtual pobj clone() const = 0;
	string toString ( );
};

#define OBJ_IMPL(type, getter) \
class type##_obj : public obj { \
	std::shared_ptr<type> data; \
public: \
	type##_obj(const type& o = type()) { data = std::make_shared<type>(); *data = o; } \
	type##_obj(const std::shared_ptr<type> o) : data(o) { }  \
	virtual std::shared_ptr<type> getter() { \
	/*trace( "queried object of type "<<type_str()<<" and value "<<toString());*/ \
	return data; } \
	virtual string type_str() const { return #type; } \
	virtual bool equals(const obj& o) const { \
		if ( type_str() != o.type_str() ) return false; \
		auto od = ((const type##_obj&)o).data; \
		if ( !data || !od) return data == od; \
		return *data == *od; \
	}\
	virtual pobj clone() const { return std::make_shared<type##_obj>(*data);  }\
};typedef std::shared_ptr<type##_obj> p##type##_obj

OBJ_IMPL ( int64_t, INT );
OBJ_IMPL ( uint64_t, UINT );
OBJ_IMPL ( bool, BOOL );
OBJ_IMPL ( double, DOUBLE );
OBJ_IMPL ( string, STR );
OBJ_IMPL ( somap, MAP );
OBJ_IMPL ( olist, LIST );
OBJ_IMPL ( null, Null );

typedef obj::pobj pobj;
typedef obj::somap somap;
typedef obj::olist olist;
typedef std::shared_ptr<somap> psomap;
typedef std::shared_ptr<olist> polist;
typedef std::map<string, bool> defined_t;
typedef std::shared_ptr<defined_t> pdefined_t;

template<typename T> inline pstring_obj mk_str_obj ( T t ) {
	return std::make_shared<string_obj> ( t );
}

template<typename T> inline psomap_obj mk_somap_obj ( T t ) {
	return std::make_shared<somap_obj> ( t );
}

template<typename T> inline polist_obj mk_olist_obj ( T t ) {
	return std::make_shared<olist_obj> ( t );
}

template<typename T> inline polist mk_olist ( T t ) {
	return std::make_shared<olist> ( t );
}

pstring_obj mk_str_obj();
psomap_obj mk_somap_obj();
polist_obj mk_olist_obj();
polist mk_olist();
bool has ( const defined_t& c, const string& k );
bool has ( pdefined_t c, const string& k );
bool has ( const somap& c, const string& k );
bool has ( psomap c, const string& k );
bool has ( psomap c, pstring k );
bool has ( pobj o, string s );
bool has ( pobj o, pstring s );

namespace jsonld {
// http://www.w3.org/TR/json-ld-api/#the-jsonldoptions-type
struct jsonld_options {
	jsonld_options() {
	}

	jsonld_options ( pstring base_ ) : base ( base_ ) {}
	jsonld_options ( string base_ ) :
		base ( pstr ( base_ ) ) {
	}

	pstring base = 0;//pstr ( "http://tauchain.org/" );
	pbool compactArrays = std::make_shared<bool> ( true );
	std::shared_ptr<class obj> expandContext = 0;
	pstring processingMode = pstr ( "json-ld-1.0" );
	pbool embed = 0;
	pbool isexplicit = std::make_shared<bool> ( true );
	pbool omitDefault = 0;
	pbool useRdfType = std::make_shared<bool> ( true );
	pbool useNativeTypes = std::make_shared<bool> ( true );
	pbool produceGeneralizedRdf = std::make_shared<bool> ( true );
	pstring format = 0;
	pbool useNamespaces = std::make_shared<bool> ( true );
	pstring outputForm = 0;
};
}
#endif
