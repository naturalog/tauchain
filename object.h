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

//#include "logger.h"
extern bool deref;
//#define DEBUG
//#define VERBOSE
#ifdef DEBUG
//logger _logger;
extern bool autobt, _pause;
inline void bt() {
	void *trace[16];
	char **messages = 0;
	int i, trace_size = 0;
	trace_size = backtrace ( trace, 16 );
	trace[1] = ( void * ) __builtin_return_address ( 0 );
	messages = backtrace_symbols ( trace, trace_size );
	printf ( "[bt] Execution path:\n" );
	for ( i = 1; i < trace_size; ++i ) {
		printf ( "[bt] #%d %s\n", i, messages[i] );
		size_t p = 0;
		while ( messages[i][p] != '(' && messages[i][p] != ' '
		        && messages[i][p] != 0 )
			++p;
		char syscom[256];
		sprintf ( syscom, "addr2line %p -e %.*s", trace[i], ( int ) p, messages[i] );
		system ( syscom );
	}
}
inline void dopause() {
	std::clog << "press any key to continue, b for backtrace, or a to always show backtrace, or c to stop pausing...";
	char ch = getchar();
	if ( ch == 'b' || ( autobt = ( ch == 'a' ) ) ) bt();
	else if ( ch == 'c' ) autobt = _pause = false;
}


#define trace(x) std::clog<<x; if (_pause) dopause()
#else
inline void bt() {}
#define trace(x)
#endif

typedef std::nullptr_t null;
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
	size_t size() {
		if ( LIST() ) return LIST()->size();
		if ( MAP() ) return MAP()->size();
		return 1;
	}
	bool STR ( const string& x ) {
		auto y = STR();
		return y && ( *y == x );
	}
	std::shared_ptr<obj> MAP ( const string& k ) {
		auto y = MAP();
		if ( !y ) return 0;
		somap::iterator it = y->find ( k );
		return it == y->end() ? 0 : it->second;
	}

	bool map_and_has ( const string& k ) {
		psomap m = MAP();
		return m && m->find ( k ) != m->end();
	}
	bool map_and_has_null ( const string& k ) {
		psomap m = MAP();
		if ( !m ) return false;
		auto it = m->find ( k );
		return ( it != m->end() ) && ( !it->second || it->second->Null() );
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
typedef std::shared_ptr<bool> pbool;
typedef std::map<string, bool> defined_t;
typedef std::shared_ptr<defined_t> pdefined_t;

template<typename T> inline pstring_obj mk_str_obj ( T t ) {
	return std::make_shared<string_obj> ( t );
}

inline pstring_obj mk_str_obj() {
	return std::make_shared<string_obj>();
}

template<typename T> inline psomap_obj mk_somap_obj ( T t ) {
	return std::make_shared<somap_obj> ( t );
}

inline psomap_obj mk_somap_obj() {
	return std::make_shared<somap_obj>();
}

template<typename T> inline polist_obj mk_olist_obj ( T t ) {
	return std::make_shared<olist_obj> ( t );
}

inline polist_obj mk_olist_obj() {
	return std::make_shared<olist_obj>();
}

template<typename T> inline polist mk_olist ( T t ) {
	return std::make_shared<olist> ( t );
}

inline polist mk_olist() {
	return std::make_shared<olist>();
}

inline bool has ( const defined_t& c, const string& k ) {
	return c.find ( k ) != c.end();
}

inline bool has ( pdefined_t c, const string& k ) {
	return c && has ( *c, k );
}

inline bool has ( const somap& c, const string& k ) {
	#ifdef VERBOSE
	trace ( "query for key " << k << "form object: " << std::endl << mk_somap_obj ( c )->toString() << std::endl );
	#endif
	return c.find ( k ) != c.end();
}

inline bool has ( psomap c, const string& k ) {
	return c && has ( *c, k );
}

inline bool has ( psomap c, pstring k ) {
	return k && has ( c, *k );
}

inline bool has ( pobj o, string s ) {
	return o && o->MAP() && has ( o->MAP(), s );
}

inline bool has ( pobj o, pstring s ) {
	return s && has ( o, *s );
}

#endif
