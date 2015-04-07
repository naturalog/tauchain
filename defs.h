#ifndef __DEFS_H__
#define __DEFS_H__

#define JSON_SPIRIT_MVALUE_ENABLED

#include <string>
#include <cstring>
#include <locale>
#include <boost/variant/variant.hpp>
#include <map>
#include <list>
//#include <regex>
#include <stdexcept>
#include <boost/regex.hpp>
#include <algorithm>
#include <utility>
#include <cctype>
using namespace std::string_literals;

namespace json_spirit {
struct Null {
	template<typename T>
	bool operator== ( const T& ) {
		return false;
	}
	bool operator== ( const Null& ) {
		return true;
	}
};
}
json_spirit::Null null;

template<typename K, typename V>
class map_t : public std::map<K, V> {
public:
	typedef std::map<K, V> base_t;
	using base_t::base_t;
	map_t() : base_t() {}
	bool has ( const K& k ) {
		auto cached = base_t::find ( k );
		return cached != base_t::cend();
	}
	V& put ( const K& k, const V& v ) {
		return at ( k ) = v;
	}
	//	void put(const K& k, const V& v) { at(k) = v; }
	V& get ( const K& k ) {
		return /*(cached != base_t::cend() && cached.first == k) ? cached.second :*/ at ( k );
	}
	const V& get ( const K& k ) const {
		return /*(cached != base_t::cend() && cached.first == k) ? cached.second :*/ at ( k );
	}
	bool get ( const K& k, V& v ) {
		auto it = find ( k );
		if ( it != base_t::end() ) {
			v = it->second;
			return true;
		}
		return false;
	}
	//	V& get ( const K& k ) {
	//		auto it = find ( k );
	//		if ( it != base_t::end() ) return it->second;
	//		return "";
	//	}
	bool containsKey ( const K& k ) const {
		return find ( k ) != base_t::end();
	}
	const V& remove ( const K& k ) {
		auto it = find ( k );
		if ( it == base_t::end() ) return null;
		auto v = it->second;
		erase ( it );
		return v;
	}
	virtual bool isContext() const {
		return false;
	}
	//private:
	//	decltype(base_t::cend()) cached = base_t::cend();
};

template<typename K, typename V> using Map = map_t<K, V>;
template<typename K, typename V> using HashMap = map_t<K, V>;
template<typename K, typename V> using LinkedHashMap = map_t<K, V>;
template<typename K> using List = std::vector<K>;
template<typename K> using ArrayList = std::vector<K>;

#include <boost/logic/tribool.hpp>
BOOST_TRIBOOL_THIRD_STATE ( bnull )

typedef std::string String;
typedef bool boolean;
typedef bool Boolean;
typedef boost::tribool tBoolean;
typedef double Double;
typedef int64_t Integer;
inline bool is ( const String& s, const std::vector<String>& v ) {
	return std::find ( v.begin(), v.end(), s ) != v.end();
}

inline String lower ( const String& s_ ) {
	String s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::tolower );
	return s;
}

typedef std::runtime_error NullPointerException;

inline bool endsWith ( const String& x, const String& y ) {
	return x.size() >= y.size() && x.substr ( x.size() - y.size(), y.size() ) == y;
}
inline bool startsWith ( const String& x, const String& y ) {
	return x.size() >= y.size() && x.substr ( 0, y.size() ) == y;
}

template<typename charT>
inline List<String> split ( const String& s, charT c ) {
	List<String> v;
	for ( String::size_type i = 0,  j = s.find ( c ); j != String::npos; ) {
		v.push_back ( s.substr ( i, j - i ) );
		j = s.find ( c, i = ++j );
		if ( j == String::npos ) v.push_back ( s.substr ( i, s.length() ) );
	}
	return v;
}
#endif
