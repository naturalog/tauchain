#ifndef __DEFS_H__
#define __DEFS_H__

#define JSON_SPIRIT_MVALUE_ENABLED

#include <string>
#include <boost/variant/variant.hpp>
#include <map>
#include <list>
//#include <regex>
#include <stdexcept>
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
	void put ( const K& k, const V& v ) {
		at ( k ) = v;
	}
	//	void put(const K& k, const V& v) { at(k) = v; }
	V& get ( const K& k ) {
		return at ( k );
	}
	const V& get ( const K& k ) const {
		return at ( k );
	}
	bool get ( const K& k, V& v ) {
		auto it = find ( k );
		if ( it != base_t::end() ) {
			v = it->second;
			return true;
		} return false;
	}
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
	map_t() : base_t() {}
	virtual bool isContext() const {
		return false;
	}
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
typedef boost::tribool Boolean;

inline bool is ( const String& s, const std::vector<String>& v ) {
	return std::find ( v.begin(), v.end(), s ) != v.end();
}

//typedef std::basic_regex<String> Pattern;
/*
    namespace json_spirit {
    class mValue;
    }
    typedef json_spirit::mValue Object;
*/
#endif
