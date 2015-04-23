#include <map>
#include <vector>
#include <memory>
#include <string>
//#include "logger.h"

#define DEBUG
#ifdef DEBUG
//logger _logger;
auto dummy = []() {
	return ( bool ) std::cin.tie ( &std::clog );
}();
void dopause() {
	std::clog << "press any key to continue...";
	getchar();
}
bool _pause = false;
#define trace(x) std::clog<<x; if (_pause) dopause()
#else
#define trace(x)
#endif

typedef nullptr_t null;
typedef std::string string;
typedef std::shared_ptr<string> pstring;

inline pstring pstr ( const string& s ) {
	return std::make_shared<string> ( s );
}
inline pstring pstr ( const char* s ) {
	return s ? pstr ( string ( s ) ) : 0;
}
inline pstring pstr ( const unsigned char* s ) {
	return pstr ( ( const char* ) s );
}

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

	bool map_and_has ( const string& k ) {
		psomap m = MAP();
		return m && m->find ( k ) != m->end();
	}
	bool map_and_has_null ( const string& k ) {
		psomap m = MAP();
		if ( !m ) return false;
		auto it = m->find ( k );
		return it != m->end() && it->second->Null();
	}
	virtual string type_str() const = 0;
	virtual bool equals ( const obj& o ) const = 0;
	virtual pobj clone() const = 0;
	string toString ( );
};

#define OBJ_IMPL(type, getter) \
class type##_obj : public obj { \
	std::shared_ptr<type> data; \
public: \
	type##_obj(const type& o = type()) { data = std::make_shared<type>(); *data = o; \
	trace( "created object of type "<<type_str()<<" and value "<<toString());} \
	type##_obj(const std::shared_ptr<type> o) : data(o) {  \
	trace( "created object of type "<<type_str()<<" and value "<<toString());} \
	virtual std::shared_ptr<type> getter() { return data; } \
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
typedef std::shared_ptr<bool> pbool;
typedef std::map<string, bool> defined_t;
typedef std::shared_ptr<defined_t> pdefined_t;

template<typename T> pstring_obj mk_str_obj ( T t ) {
	return std::make_shared<string_obj> ( t );
}

pstring_obj mk_str_obj() {
	return std::make_shared<string_obj>();
}

template<typename T> psomap_obj mk_somap_obj ( T t ) {
	return std::make_shared<somap_obj> ( t );
}

psomap_obj mk_somap_obj() {
	return std::make_shared<somap_obj>();
}

template<typename T> polist_obj mk_olist_obj ( T t ) {
	return std::make_shared<olist_obj> ( t );
}

polist_obj mk_olist_obj() {
	return std::make_shared<olist_obj>();
}

template<typename T> polist mk_olist ( T t ) {
	return std::make_shared<olist> ( t );
}

polist mk_olist() {
	return std::make_shared<olist>();
}

bool has ( const defined_t& c, const string& k ) {
	return c.find ( k ) != c.end();
}

bool has ( pdefined_t c, const string& k ) {
	return c && has ( *c, k );
}

bool has ( const somap& c, const string& k ) {
	trace ( "query for key " << k << "form object: " << std::endl << mk_somap_obj ( c )->toString() );
	return c.find ( k ) != c.end();
}

bool has ( psomap c, const string& k ) {
	return c && has ( *c, k );
}

bool has ( psomap c, pstring k ) {
	return k && has ( c, *k );
}

bool has ( pobj o, string s ) {
	return o && o->MAP() && has ( o->MAP(), s );
}

bool has ( pobj o, pstring s ) {
	return s && has ( o, *s );
}

json_spirit::mValue convert ( obj& v ) {
	typedef json_spirit::mValue val;
	val r;
	if ( v.UINT() ) return val ( *v.UINT() );
	if ( v.INT() )  return val ( *v.INT() );
	if ( v.STR() )  return val ( *v.STR() );
	if ( v.DOUBLE() )  return val ( *v.DOUBLE() );
	if ( v.BOOL() )  return val ( *v.BOOL() );
	else if ( v.LIST() ) {
		val::Array a;
		for ( auto x : *v.LIST() ) a.push_back ( convert ( *x ) );
		return val ( a );
	} else {
		if ( !v.MAP() ) throw "logic error";
		val::Object a;
		for ( auto x : *v.MAP() ) a[x.first] = convert ( *x.second );
		return val ( a );
	}
}

pobj convert ( const json_spirit::mValue& v ) {
	using namespace std;
	pobj r;
	if ( v.is_uint64() ) r = make_shared<uint64_t_obj> ( v.get_uint64() );
	else if ( v.isInt() ) r = make_shared<int64_t_obj> ( v.get_int64() );
	else if ( v.isString() ) r = make_shared<string_obj> ( v.get_str() );
	else if ( v.isDouble() ) r = make_shared<double_obj> ( v.get_real() );
	else if ( v.isBoolean() ) r = make_shared<bool_obj> ( v.get_bool() );
	else if ( v.is_null() ) r = 0;
	else if ( v.isList() ) {
		r = make_shared<olist_obj>();
		auto a = v.get_array();
		for ( auto x : a ) r->LIST()->push_back ( convert ( x ) );
	} else {
		if ( !v.isMap() ) throw "logic error";
		r = make_shared<somap_obj>();
		auto a = v.get_obj();
		for ( auto x : a ) ( *r->MAP() ) [x.first] = convert ( x.second );
	}
	return r;
}
