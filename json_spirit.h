#ifndef JSON_SPIRIT_VALUE
#define JSON_SPIRIT_VALUE

//          Copyright John W. Wilkinson 2007 - 2014
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.08

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION >= 103800
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_escape_char.hpp>
#include <boost/spirit/include/classic_multi_pass.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#define spirit_namespace boost::spirit::classic
#else
#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/escape_char.hpp>
#include <boost/spirit/iterator/multi_pass.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#define spirit_namespace boost::spirit
#endif
#include <cassert>
#include <sstream>
#include <iomanip>
#include <boost/io/ios_state.hpp>

// comment out the value types you don't need to reduce build times and intermediate file sizes
//#define JSON_SPIRIT_VALUE_ENABLED
//#define JSON_SPIRIT_WVALUE_ENABLED
#define JSON_SPIRIT_MVALUE_ENABLED
//#define JSON_SPIRIT_WMVALUE_ENABLED

namespace json_spirit {
enum Value_type { obj_type, array_type, str_type, bool_type, int_type, real_type, null_type };

static std::string value_type_to_string ( Value_type vtype );

struct Null {};

template<class Config>    // Config determines whether the value uses std::string or std::wstring and
// whether JSON Objects are represented as vectors or maps
class Value_impl {
public:

	typedef Config Config_type;
	typedef typename Config::String_type String_type;
	typedef typename Config::Object_type Object;
	typedef typename Config::Array_type Array;
	typedef typename String_type::const_pointer Const_str_ptr;  // eg const char*

	Value_impl();  // creates null value
	Value_impl ( Const_str_ptr      value );
	Value_impl ( const String_type& value );
	Value_impl ( const Object&      value );
	Value_impl ( const Array&       value );
	Value_impl ( bool               value );
	Value_impl ( int                value );
	Value_impl ( boost::int64_t     value );
	Value_impl ( boost::uint64_t    value );
	Value_impl ( double             value );

	template<class Iter>
	Value_impl ( Iter first, Iter last );   // constructor from containers, e.g. std::vector or std::list

	template<BOOST_VARIANT_ENUM_PARAMS ( typename T ) >
	Value_impl ( const boost::variant<BOOST_VARIANT_ENUM_PARAMS ( T ) >& variant ); // constructor for compatible variant types

	Value_impl ( const Value_impl& other );

	bool operator== ( const Value_impl& lhs ) const;

	Value_impl& operator= ( const Value_impl& lhs );

	Value_type type() const;

	bool is_uint64() const;
	bool is_null() const;

	const String_type& get_str()    const;
	const Object&      get_obj()    const;
	const Array&       get_array()  const;
	bool               get_bool()   const;
	int                get_int()    const;
	boost::int64_t     get_int64()  const;
	boost::uint64_t    get_uint64() const;
	double             get_real()   const;

	bool isMap() const {
		return type() == obj_type;
	}
	bool isList() const {
		return type() == array_type;
	}
	bool isString() const {
		return type() == str_type;
	}
	bool isBoolean() const {
		return type() == bool_type;
	}
	bool isInt() const {
		return type() == int_type;
	}
	bool isDouble() const {
		return type() == real_type;
	}
	bool isNumber() const {
		return type() == int_type || type() == real_type || is_uint64();
	}

	Object& get_obj();
	Array&  get_array();

	template<typename T> T get_value() const;  // example usage: int    i = value.get_value< int >();
	// or             double d = value.get_value< double >();

	static const Value_impl null;

private:

	void check_type ( const Value_type vtype ) const;

	typedef boost::variant<boost::recursive_wrapper<Object>, boost::recursive_wrapper<Array>,
	        String_type, bool, boost::int64_t, double, Null, boost::uint64_t> Variant;

	Variant v_;

	class Variant_converter_visitor : public boost::static_visitor<Variant> {
	public:

		template<typename T, typename A, template<typename, typename> class Cont>
		Variant operator() ( const Cont<T, A>& cont ) const {
			return Array ( cont.begin(), cont.end() );
		}

		Variant operator() ( int i ) const {
			return static_cast<boost::int64_t> ( i );
		}

		template<class T>
		Variant operator() ( const T& t ) const {
			return t;
		}
	};
};

// vector objects

template<class Config>
struct Pair_impl {
	typedef typename Config::String_type String_type;
	typedef typename Config::Value_type Value_type;

	Pair_impl() {
	}

	Pair_impl ( const String_type& name, const Value_type& value );

	bool operator== ( const Pair_impl& lhs ) const;

	String_type name_;
	Value_type value_;
};

#if defined( JSON_SPIRIT_VALUE_ENABLED ) || defined( JSON_SPIRIT_WVALUE_ENABLED )
template<class String>
struct Config_vector {
	typedef String String_type;
	typedef Value_impl<Config_vector> Value_type;
	typedef Pair_impl <Config_vector> Pair_type;
	typedef std::vector<Value_type> Array_type;
	typedef std::vector<Pair_type> Object_type;

	static Value_type& add ( Object_type& obj, const String_type& name, const Value_type& value ) {
		obj.push_back ( Pair_type ( name , value ) );

		return obj.back().value_;
	}

	static const String_type& get_name ( const Pair_type& pair ) {
		return pair.name_;
	}

	static const Value_type& get_value ( const Pair_type& pair ) {
		return pair.value_;
	}
};
#endif

// typedefs for ASCII

#ifdef JSON_SPIRIT_VALUE_ENABLED
typedef Config_vector<std::string> Config;

typedef Config::Value_type  Value;
typedef Config::Pair_type   Pair;
typedef Config::Object_type Object;
typedef Config::Array_type  Array;
#endif

// typedefs for Unicode

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
typedef Config_vector<std::wstring> wConfig;

typedef wConfig::Value_type  wValue;
typedef wConfig::Pair_type   wPair;
typedef wConfig::Object_type wObject;
typedef wConfig::Array_type  wArray;
#endif

// map objects

#if defined( JSON_SPIRIT_MVALUE_ENABLED ) || defined( JSON_SPIRIT_WMVALUE_ENABLED )
template<class String>
struct Config_map {
	typedef String String_type;
	typedef Value_impl<Config_map> Value_type;
	typedef std::vector<Value_type> Array_type;
	typedef std::map<String_type, Value_type> Object_type;
	typedef std::pair<const String_type, Value_type> Pair_type;

	static Value_type& add ( Object_type& obj, const String_type& name, const Value_type& value ) {
		return obj[ name ] = value;
	}

	static const String_type& get_name ( const Pair_type& pair ) {
		return pair.first;
	}

	static const Value_type& get_value ( const Pair_type& pair ) {
		return pair.second;
	}
};
#endif

// typedefs for ASCII

#ifdef JSON_SPIRIT_MVALUE_ENABLED
typedef Config_map<std::string> mConfig;

typedef mConfig::Value_type  mValue;
typedef mConfig::Object_type mObject;
typedef mConfig::Array_type  mArray;
#endif

// typedefs for Unicode

#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
typedef Config_map<std::wstring> wmConfig;

typedef wmConfig::Value_type  wmValue;
typedef wmConfig::Object_type wmObject;
typedef wmConfig::Array_type  wmArray;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
//
// implementation

inline bool operator== ( const Null&, const Null& ) {
	return true;
}

template<class Config>
const Value_impl<Config> Value_impl<Config>::null;

template<class Config>
Value_impl<Config>::Value_impl()
	:   v_ ( Null() ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( const Const_str_ptr value )
	:  v_ ( String_type ( value ) ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( const String_type& value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( const Object& value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( const Array& value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( bool value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( int value )
	:   v_ ( static_cast<boost::int64_t> ( value ) ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( boost::int64_t value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( boost::uint64_t value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( double value )
	:   v_ ( value ) {
}

template<class Config>
Value_impl<Config>::Value_impl ( const Value_impl<Config>& other )
	:   v_ ( other.v_ ) {
}

template<class Config>
template<class Iter>
Value_impl<Config>::Value_impl ( Iter first, Iter last )
	:   v_ ( Array ( first, last ) ) {
}

template<class Config>
template<BOOST_VARIANT_ENUM_PARAMS ( typename T ) >
Value_impl<Config>::Value_impl ( const boost::variant<BOOST_VARIANT_ENUM_PARAMS ( T ) >& variant )
	:   v_ ( boost::apply_visitor ( Variant_converter_visitor(), variant ) ) {
}

template<class Config>
Value_impl<Config>& Value_impl<Config>::operator= ( const Value_impl& lhs ) {
	Value_impl tmp ( lhs );

	std::swap ( v_, tmp.v_ );

	return *this;
}

template<class Config>
bool Value_impl<Config>::operator== ( const Value_impl& lhs ) const {
	if ( this == &lhs ) return true;

	if ( type() != lhs.type() ) return false;

	return v_ == lhs.v_;
}

template<class Config>
Value_type Value_impl<Config>::type() const {
	if ( is_uint64() )
		return int_type;

	return static_cast<Value_type> ( v_.which() );
}

template<class Config>
bool Value_impl<Config>::is_uint64() const {
	return v_.which() == null_type + 1;
}

template<class Config>
bool Value_impl<Config>::is_null() const {
	return type() == null_type;
}

template<class Config>
void Value_impl<Config>::check_type ( const Value_type vtype ) const {
	if ( type() != vtype ) {
		std::ostringstream os;

		os << "get_value< " << value_type_to_string ( vtype ) << " > called on " << value_type_to_string ( type() ) << " Value";

		throw std::runtime_error ( os.str() );
	}
}

template<class Config>
const typename Config::String_type& Value_impl<Config>::get_str() const {
	check_type ( str_type );

	return *boost::get<String_type> ( &v_ );
}

template<class Config>
const typename Value_impl<Config>::Object& Value_impl<Config>::get_obj() const {
	check_type ( obj_type );

	return *boost::get<Object> ( &v_ );
}

template<class Config>
const typename Value_impl<Config>::Array& Value_impl<Config>::get_array() const {
	check_type ( array_type );

	return *boost::get<Array> ( &v_ );
}

template<class Config>
bool Value_impl<Config>::get_bool() const {
	check_type ( bool_type );

	return boost::get<bool> ( v_ );
}

template<class Config>
int Value_impl<Config>::get_int() const {
	check_type ( int_type );

	return static_cast<int> ( get_int64() );
}

template<class Config>
boost::int64_t Value_impl<Config>::get_int64() const {
	check_type ( int_type );

	if ( is_uint64() )
		return static_cast<boost::int64_t> ( get_uint64() );

	return boost::get<boost::int64_t> ( v_ );
}

template<class Config>
boost::uint64_t Value_impl<Config>::get_uint64() const {
	check_type ( int_type );

	if ( !is_uint64() )
		return static_cast<boost::uint64_t> ( get_int64() );

	return boost::get<boost::uint64_t> ( v_ );
}

template<class Config>
double Value_impl<Config>::get_real() const {
	if ( type() == int_type ) {
		return is_uint64() ? static_cast<double> ( get_uint64() )
		       : static_cast<double> ( get_int64() );
	}

	check_type ( real_type );

	return boost::get<double> ( v_ );
}

template<class Config>
typename Value_impl<Config>::Object& Value_impl<Config>::get_obj() {
	check_type ( obj_type );

	return *boost::get<Object> ( &v_ );
}

template<class Config>
typename Value_impl<Config>::Array& Value_impl<Config>::get_array() {
	check_type ( array_type );

	return *boost::get<Array> ( &v_ );
}

template<class Config>
Pair_impl<Config>::Pair_impl ( const String_type& name, const Value_type& value )
	:   name_ ( name )
	,   value_ ( value ) {
}

template<class Config>
bool Pair_impl<Config>::operator== ( const Pair_impl<Config>& lhs ) const {
	if ( this == &lhs ) return true;

	return ( name_ == lhs.name_ ) && ( value_ == lhs.value_ );
}

// converts a C string, ie. 8 bit char array, to a string object
//
template <class String_type>
String_type to_str ( const char* c_str ) {
	String_type result;

	for ( const char* p = c_str; *p != 0; ++p )
		result += *p;

	return result;
}

//

namespace internal_ {
template<typename T>
struct Type_to_type {
};

template<class Value>
int get_value ( const Value& value, Type_to_type<int> ) {
	return value.get_int();
}

template<class Value>
boost::int64_t get_value ( const Value& value, Type_to_type<boost::int64_t> ) {
	return value.get_int64();
}

template<class Value>
boost::uint64_t get_value ( const Value& value, Type_to_type<boost::uint64_t> ) {
	return value.get_uint64();
}

template<class Value>
double get_value ( const Value& value, Type_to_type<double> ) {
	return value.get_real();
}

template<class Value>
typename Value::String_type get_value ( const Value& value, Type_to_type<typename Value::String_type> ) {
	return value.get_str();
}

template<class Value>
typename Value::Array get_value ( const Value& value, Type_to_type<typename Value::Array> ) {
	return value.get_array();
}

template<class Value>
typename Value::Object get_value ( const Value& value, Type_to_type<typename Value::Object> ) {
	return value.get_obj();
}

template<class Value>
bool get_value ( const Value& value, Type_to_type<bool> ) {
	return value.get_bool();
}
}

template<class Config>
template<typename T>
T Value_impl<Config>::get_value() const {
	return internal_::get_value ( *this, internal_::Type_to_type<T>() );
}

static std::string value_type_to_string ( const Value_type vtype ) {
	switch ( vtype ) {
		case obj_type:
			return "Object";
		case array_type:
			return "Array";
		case str_type:
			return "string";
		case bool_type:
			return "boolean";
		case int_type:
			return "integer";
		case real_type:
			return "real";
		case null_type:
			return "null";
	}

	assert ( false );

	return "unknown type";
}

// An Error_position exception is thrown by the "read_or_throw" functions below on finding an error.
// Note the "read_or_throw" functions are around 3 times slower than the standard functions "read"
// functions that return a bool.
//
struct Error_position {
	Error_position();
	Error_position ( unsigned int line, unsigned int column, const std::string& reason );
	bool operator== ( const Error_position& lhs ) const;
	unsigned int line_;
	unsigned int column_;
	std::string reason_;
};

inline Error_position::Error_position()
	:   line_ ( 0 )
	,   column_ ( 0 ) {
}

inline Error_position::Error_position ( unsigned int line, unsigned int column, const std::string& reason )
	:   line_ ( line )
	,   column_ ( column )
	,   reason_ ( reason ) {
}

inline bool Error_position::operator== ( const Error_position& lhs ) const {
	if ( this == &lhs ) return true;

	return ( reason_ == lhs.reason_ ) &&
	       ( line_   == lhs.line_ ) &&
	       ( column_ == lhs.column_ );
}

template<class Obj_t, class Map_t>
void obj_to_map ( const Obj_t& obj, Map_t& mp_obj ) {
	mp_obj.clear();

	for ( typename Obj_t::const_iterator i = obj.begin(); i != obj.end(); ++i )
		mp_obj[ i->name_ ] = i->value_;
}

template<class Obj_t, class Map_t>
void map_to_obj ( const Map_t& mp_obj, Obj_t& obj ) {
	obj.clear();

	for ( typename Map_t::const_iterator i = mp_obj.begin(); i != mp_obj.end(); ++i )
		obj.push_back ( typename Obj_t::value_type ( i->first, i->second ) );
}

#ifdef JSON_SPIRIT_VALUE_ENABLED
typedef std::map<std::string, Value> Mapped_obj;
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
typedef std::map<std::wstring, wValue> wMapped_obj;
#endif

template<class Object_type, class String_type>
const typename Object_type::value_type::Value_type& find_value ( const Object_type& obj, const String_type& name ) {
	for ( typename Object_type::const_iterator i = obj.begin(); i != obj.end(); ++i ) {
		if ( i->name_ == name )
			return i->value_;
	}

	return Object_type::value_type::Value_type::null;
}


const spirit_namespace::int_parser <boost::int64_t>  int64_p  = spirit_namespace::int_parser <boost::int64_t>();
const spirit_namespace::uint_parser<boost::uint64_t> uint64_p = spirit_namespace::uint_parser<boost::uint64_t>();

template<class Iter_type>
bool is_eq ( Iter_type first, Iter_type last, const char* c_str ) {
	for ( Iter_type i = first; i != last; ++i, ++c_str ) {
		if ( *c_str == 0 ) return false;

		if ( *i != *c_str ) return false;
	}

	return true;
}

template<class Char_type>
Char_type hex_to_num ( const Char_type c ) {
	if ( ( c >= '0' ) && ( c <= '9' ) ) return c - '0';
	if ( ( c >= 'a' ) && ( c <= 'f' ) ) return c - 'a' + 10;
	if ( ( c >= 'A' ) && ( c <= 'F' ) ) return c - 'A' + 10;
	return 0;
}

template<class Char_type, class Iter_type>
Char_type hex_str_to_char ( Iter_type& begin ) {
	const Char_type c1 ( * ( ++begin ) );
	const Char_type c2 ( * ( ++begin ) );

	return ( hex_to_num ( c1 ) << 4 ) + hex_to_num ( c2 );
}

template<class Char_type, class Iter_type>
Char_type unicode_str_to_char ( Iter_type& begin ) {
	const Char_type c1 ( * ( ++begin ) );
	const Char_type c2 ( * ( ++begin ) );
	const Char_type c3 ( * ( ++begin ) );
	const Char_type c4 ( * ( ++begin ) );

	return ( hex_to_num ( c1 ) << 12 ) +
	       ( hex_to_num ( c2 ) <<  8 ) +
	       ( hex_to_num ( c3 ) <<  4 ) +
	       hex_to_num ( c4 );
}

template<class String_type>
void append_esc_char_and_incr_iter ( String_type& s,
                                     typename String_type::const_iterator& begin,
                                     typename String_type::const_iterator end ) {
	typedef typename String_type::value_type Char_type;

	const Char_type c2 ( *begin );

	switch ( c2 ) {
		case 't':
			s += '\t';
			break;
		case 'b':
			s += '\b';
			break;
		case 'f':
			s += '\f';
			break;
		case 'n':
			s += '\n';
			break;
		case 'r':
			s += '\r';
			break;
		case '\\':
			s += '\\';
			break;
		case '/':
			s += '/';
			break;
		case '"':
			s += '"';
			break;
		case 'x': {
				if ( end - begin >= 3 ) //  expecting "xHH..."
					s += hex_str_to_char<Char_type> ( begin );
				break;
			}
		case 'u': {
				if ( end - begin >= 5 ) //  expecting "uHHHH..."
					s += unicode_str_to_char<Char_type> ( begin );
				break;
			}
	}
}

template<class String_type>
String_type substitute_esc_chars ( typename String_type::const_iterator begin,
                                   typename String_type::const_iterator end ) {
	typedef typename String_type::const_iterator Iter_type;

	if ( end - begin < 2 ) return String_type ( begin, end );

	String_type result;

	result.reserve ( end - begin );

	const Iter_type end_minus_1 ( end - 1 );

	Iter_type substr_start = begin;
	Iter_type i = begin;

	for ( ; i < end_minus_1; ++i ) {
		if ( *i == '\\' ) {
			result.append ( substr_start, i );

			++i;  // skip the '\'

			append_esc_char_and_incr_iter ( result, i, end );

			substr_start = i + 1;
		}
	}

	result.append ( substr_start, end );

	return result;
}

template<class String_type>
String_type get_str_ ( typename String_type::const_iterator begin,
                       typename String_type::const_iterator end ) {
	assert ( end - begin >= 2 );

	typedef typename String_type::const_iterator Iter_type;

	Iter_type str_without_quotes ( ++begin );
	Iter_type end_without_quotes ( --end );

	return substitute_esc_chars<String_type> ( str_without_quotes, end_without_quotes );
}

inline std::string get_str ( std::string::const_iterator begin, std::string::const_iterator end ) {
	return get_str_<std::string> ( begin, end );
}

inline std::wstring get_str ( std::wstring::const_iterator begin, std::wstring::const_iterator end ) {
	return get_str_<std::wstring> ( begin, end );
}

template<class String_type, class Iter_type>
String_type get_str ( Iter_type begin, Iter_type end ) {
	const String_type tmp ( begin, end ); // convert multipass iterators to string iterators

	return get_str ( tmp.begin(), tmp.end() );
}

// this class's methods get called by the spirit parse resulting
// in the creation of a JSON object or array
//
// NB Iter_type could be a std::string iterator, wstring iterator, a position iterator or a multipass iterator
//
template<class Value_type, class Iter_type>
class Semantic_actions {
public:

	typedef typename Value_type::Config_type Config_type;
	typedef typename Config_type::String_type String_type;
	typedef typename Config_type::Object_type Object_type;
	typedef typename Config_type::Array_type Array_type;
	typedef typename String_type::value_type Char_type;

	Semantic_actions ( Value_type& value )
		:   value_ ( value )
		,   current_p_ ( 0 ) {
	}

	void begin_obj ( Char_type c ) {
		assert ( c == '{' );

		begin_compound<Object_type>();
	}

	void end_obj ( Char_type c ) {
		assert ( c == '}' );

		end_compound();
	}

	void begin_array ( Char_type c ) {
		assert ( c == '[' );

		begin_compound<Array_type>();
	}

	void end_array ( Char_type c ) {
		assert ( c == ']' );

		end_compound();
	}

	void new_name ( Iter_type begin, Iter_type end ) {
		assert ( current_p_->type() == obj_type );

		name_ = get_str<String_type> ( begin, end );
	}

	void new_str ( Iter_type begin, Iter_type end ) {
		add_to_current ( get_str<String_type> ( begin, end ) );
	}

	void new_true ( Iter_type begin, Iter_type end ) {
		assert ( is_eq ( begin, end, "true" ) );

		add_to_current ( true );
	}

	void new_false ( Iter_type begin, Iter_type end ) {
		assert ( is_eq ( begin, end, "false" ) );

		add_to_current ( false );
	}

	void new_null ( Iter_type begin, Iter_type end ) {
		assert ( is_eq ( begin, end, "null" ) );

		add_to_current ( Value_type() );
	}

	void new_int ( boost::int64_t i ) {
		add_to_current ( i );
	}

	void new_uint64 ( boost::uint64_t ui ) {
		add_to_current ( ui );
	}

	void new_real ( double d ) {
		add_to_current ( d );
	}

private:

	Semantic_actions& operator= ( const Semantic_actions& );
	// to prevent "assignment operator could not be generated" warning

	Value_type* add_first ( const Value_type& value ) {
		assert ( current_p_ == 0 );

		value_ = value;
		current_p_ = &value_;
		return current_p_;
	}

	template<class Array_or_obj>
	void begin_compound() {
		if ( current_p_ == 0 )
			add_first ( Array_or_obj() );
		else {
			stack_.push_back ( current_p_ );

			Array_or_obj new_array_or_obj;   // avoid copy by building new array or object in place

			current_p_ = add_to_current ( new_array_or_obj );
		}
	}

	void end_compound() {
		if ( current_p_ != &value_ ) {
			current_p_ = stack_.back();

			stack_.pop_back();
		}
	}

	Value_type* add_to_current ( const Value_type& value ) {
		if ( current_p_ == 0 )
			return add_first ( value );
		else if ( current_p_->type() == array_type ) {
			current_p_->get_array().push_back ( value );

			return &current_p_->get_array().back();
		}

		assert ( current_p_->type() == obj_type );

		return &Config_type::add ( current_p_->get_obj(), name_, value );
	}

	Value_type& value_;             // this is the object or array that is being created
	Value_type* current_p_;         // the child object or array that is currently being constructed

	std::vector<Value_type*> stack_;   // previous child objects and arrays

	String_type name_;              // of current name/value pair
};

template<typename Iter_type>
void throw_error ( spirit_namespace::position_iterator<Iter_type> i, const std::string& reason ) {
	throw Error_position ( i.get_position().line, i.get_position().column, reason );
}

template<typename Iter_type>
void throw_error ( Iter_type i, const std::string& reason ) {
	throw reason;
}

// the spirit grammer
//
template<class Value_type, class Iter_type>
class Json_grammer : public spirit_namespace::grammar<Json_grammer<Value_type, Iter_type>> {
public:

	typedef Semantic_actions<Value_type, Iter_type> Semantic_actions_t;

	Json_grammer ( Semantic_actions_t& semantic_actions )
		:   actions_ ( semantic_actions ) {
	}

	static void throw_not_value ( Iter_type begin, Iter_type end ) {
		throw_error ( begin, "not a value" );
	}

	static void throw_not_array ( Iter_type begin, Iter_type end ) {
		throw_error ( begin, "not an array" );
	}

	static void throw_not_object ( Iter_type begin, Iter_type end ) {
		throw_error ( begin, "not an object" );
	}

	static void throw_not_pair ( Iter_type begin, Iter_type end ) {
		throw_error ( begin, "not a pair" );
	}

	static void throw_not_colon ( Iter_type begin, Iter_type end ) {
		throw_error ( begin, "no colon in pair" );
	}

	static void throw_not_string ( Iter_type begin, Iter_type end ) {
		throw_error ( begin, "not a string" );
	}

	template<typename ScannerT>
	class definition {
	public:

		definition ( const Json_grammer& self ) {
			using namespace spirit_namespace;

			typedef typename Value_type::String_type::value_type Char_type;

			// first we convert the semantic action class methods to functors with the
			// parameter signature expected by spirit

			typedef boost::function<void ( Char_type ) > Char_action;
			typedef boost::function<void ( Iter_type, Iter_type ) > Str_action;
			typedef boost::function<void ( double ) > Real_action;
			typedef boost::function<void ( boost::int64_t ) > Int_action;
			typedef boost::function<void ( boost::uint64_t ) > Uint64_action;

			Char_action   begin_obj  ( boost::bind ( &Semantic_actions_t::begin_obj,   &self.actions_, _1 ) );
			Char_action   end_obj    ( boost::bind ( &Semantic_actions_t::end_obj,     &self.actions_, _1 ) );
			Char_action   begin_array ( boost::bind ( &Semantic_actions_t::begin_array, &self.actions_, _1 ) );
			Char_action   end_array  ( boost::bind ( &Semantic_actions_t::end_array,   &self.actions_, _1 ) );
			Str_action    new_name   ( boost::bind ( &Semantic_actions_t::new_name,    &self.actions_, _1, _2 ) );
			Str_action    new_str    ( boost::bind ( &Semantic_actions_t::new_str,     &self.actions_, _1, _2 ) );
			Str_action    new_true   ( boost::bind ( &Semantic_actions_t::new_true,    &self.actions_, _1, _2 ) );
			Str_action    new_false  ( boost::bind ( &Semantic_actions_t::new_false,   &self.actions_, _1, _2 ) );
			Str_action    new_null   ( boost::bind ( &Semantic_actions_t::new_null,    &self.actions_, _1, _2 ) );
			Real_action   new_real   ( boost::bind ( &Semantic_actions_t::new_real,    &self.actions_, _1 ) );
			Int_action    new_int    ( boost::bind ( &Semantic_actions_t::new_int,     &self.actions_, _1 ) );
			Uint64_action new_uint64 ( boost::bind ( &Semantic_actions_t::new_uint64,  &self.actions_, _1 ) );

			// actual grammer

			json_
			    = value_ | eps_p[ &throw_not_value ]
			      ;

			value_
			    = string_[ new_str ]
			      | number_
			      | object_
			      | array_
			      | str_p ( "true" ) [ new_true  ]
			      | str_p ( "false" ) [ new_false ]
			      | str_p ( "null" ) [ new_null  ]
			      ;

			object_
			    = ch_p ( '{' ) [ begin_obj ]
			      >> !members_
			      >> ( ch_p ( '}' ) [ end_obj ] | eps_p[ &throw_not_object ] )
			      ;

			members_
			    = pair_ >> * ( ',' >> pair_ )
			      ;

			pair_
			    = string_[ new_name ]
			      >> ( ':' | eps_p[ &throw_not_colon ] )
			      >> ( value_ | eps_p[ &throw_not_value ] )
			      ;

			array_
			    = ch_p ( '[' ) [ begin_array ]
			      >> !elements_
			      >> ( ch_p ( ']' ) [ end_array ] | eps_p[ &throw_not_array ] )
			      ;

			elements_
			    = value_ >> * ( ',' >> value_ )
			      ;

			string_
			    = lexeme_d // this causes white space and what would appear to be comments inside a string to be retained
			      [
			          confix_p
			          (
			              '"',
			              *lex_escape_ch_p,
			              '"'
			          )
			      ]
			      ;

			number_
			    = strict_real_p[ new_real   ]
			      | int64_p      [ new_int    ]
			      | uint64_p     [ new_uint64 ]
			      ;
		}

		spirit_namespace::rule<ScannerT> json_, object_, members_, pair_, array_, elements_, value_, string_, number_;

		const spirit_namespace::rule<ScannerT>& start() const {
			return json_;
		}
	};

private:

	Json_grammer& operator= ( const Json_grammer& ); // to prevent "assignment operator could not be generated" warning

	Semantic_actions_t& actions_;
};

template<class Iter_type, class Value_type>
void add_posn_iter_and_read_range_or_throw ( Iter_type begin, Iter_type end, Value_type& value ) {
	typedef spirit_namespace::position_iterator<Iter_type> Posn_iter_t;

	const Posn_iter_t posn_begin ( begin, end );
	const Posn_iter_t posn_end ( end, end );

	read_range_or_throw ( posn_begin, posn_end, value );
}

template<class Istream_type>
struct Multi_pass_iters {
	typedef typename Istream_type::char_type Char_type;
	typedef std::istream_iterator<Char_type, Char_type> istream_iter;
	typedef spirit_namespace::multi_pass<istream_iter> Mp_iter;

	Multi_pass_iters ( Istream_type& is ) {
		is.unsetf ( std::ios::skipws );

		begin_ = spirit_namespace::make_multi_pass ( istream_iter ( is ) );
		end_   = spirit_namespace::make_multi_pass ( istream_iter() );
	}

	Mp_iter begin_;
	Mp_iter end_;
};

// reads a JSON Value from a pair of input iterators throwing an exception on invalid input, e.g.
//
// string::const_iterator start = str.begin();
// const string::const_iterator next = read_range_or_throw( str.begin(), str.end(), value );
//
// The iterator 'next' will point to the character past the
// last one read.
//
template<class Iter_type, class Value_type>
Iter_type read_range_or_throw ( Iter_type begin, Iter_type end, Value_type& value ) {
	Semantic_actions<Value_type, Iter_type> semantic_actions ( value );

	const spirit_namespace::parse_info<Iter_type> info =
	    spirit_namespace::parse ( begin, end,
	                              Json_grammer<Value_type, Iter_type> ( semantic_actions ),
	                              spirit_namespace::space_p |
	                              spirit_namespace::comment_p ( "//" ) |
	                              spirit_namespace::comment_p ( "/*", "*/" ) );

	if ( !info.hit ) {
		assert ( false ); // in theory exception should already have been thrown
		throw_error ( info.stop, "error" );
	}

	return info.stop;
}

// reads a JSON Value from a pair of input iterators, e.g.
//
// string::const_iterator start = str.begin();
// const bool success = read_string( start, str.end(), value );
//
// The iterator 'start' will point to the character past the
// last one read.
//
template<class Iter_type, class Value_type>
bool read_range ( Iter_type& begin, Iter_type end, Value_type& value ) {
	try {
		begin = read_range_or_throw ( begin, end, value );

		return true;
	} catch ( ... ) {
		return false;
	}
}

// reads a JSON Value from a string, e.g.
//
// const bool success = read_string( str, value );
//
template<class String_type, class Value_type>
bool read_string ( const String_type& s, Value_type& value ) {
	typename String_type::const_iterator begin = s.begin();

	return read_range ( begin, s.end(), value );
}

// reads a JSON Value from a string throwing an exception on invalid input, e.g.
//
// read_string_or_throw( is, value );
//
template<class String_type, class Value_type>
void read_string_or_throw ( const String_type& s, Value_type& value ) {
	add_posn_iter_and_read_range_or_throw ( s.begin(), s.end(), value );
}

// reads a JSON Value from a stream, e.g.
//
// const bool success = read_stream( is, value );
//
template<class Istream_type, class Value_type>
bool read_stream ( Istream_type& is, Value_type& value ) {
	Multi_pass_iters<Istream_type> mp_iters ( is );

	return read_range ( mp_iters.begin_, mp_iters.end_, value );
}

// reads a JSON Value from a stream throwing an exception on invalid input, e.g.
//
// read_stream_or_throw( is, value );
//
template<class Istream_type, class Value_type>
void read_stream_or_throw ( Istream_type& is, Value_type& value ) {
	const Multi_pass_iters<Istream_type> mp_iters ( is );

	add_posn_iter_and_read_range_or_throw ( mp_iters.begin_, mp_iters.end_, value );
}

enum Output_options { none = 0,             // default options

                      pretty_print = 0x01,   // Add whitespace to format the output nicely.

                      raw_utf8 = 0x02,       // This prevents non-printable characters from being escapted using "\uNNNN" notation.
                      // Note, this is an extension to the JSON standard. It disables the escaping of
                      // non-printable characters allowing UTF-8 sequences held in 8 bit char strings
                      // to pass through unaltered.

                      remove_trailing_zeros = 0x04,
                      // no longer used kept for backwards compatibility
                      single_line_arrays = 0x08,
                      // pretty printing except that arrays printed on single lines unless they contain
                      // composite elements, i.e. objects or arrays
                      always_escape_nonascii = 0x10,
                      // all unicode wide characters are escaped, i.e. outputed as "\uXXXX", even if they are
                      // printable under the current locale, ascii printable chars are not escaped
                    };
inline char to_hex_char ( unsigned int c ) {
	assert ( c <= 0xF );

	const char ch = static_cast<char> ( c );

	if ( ch < 10 ) return '0' + ch;

	return 'A' - 10 + ch;
}

template<class String_type>
String_type non_printable_to_string ( unsigned int c ) {
	String_type result ( 6, '\\' );

	result[1] = 'u';

	result[ 5 ] = to_hex_char ( c & 0x000F );
	c >>= 4;
	result[ 4 ] = to_hex_char ( c & 0x000F );
	c >>= 4;
	result[ 3 ] = to_hex_char ( c & 0x000F );
	c >>= 4;
	result[ 2 ] = to_hex_char ( c & 0x000F );

	return result;
}

template<typename Char_type, class String_type>
bool add_esc_char ( Char_type c, String_type& s ) {
	switch ( c ) {
		case '"':
			s += to_str<String_type> ( "\\\"" );
			return true;
		case '\\':
			s += to_str<String_type> ( "\\\\" );
			return true;
		case '\b':
			s += to_str<String_type> ( "\\b"  );
			return true;
		case '\f':
			s += to_str<String_type> ( "\\f"  );
			return true;
		case '\n':
			s += to_str<String_type> ( "\\n"  );
			return true;
		case '\r':
			s += to_str<String_type> ( "\\r"  );
			return true;
		case '\t':
			s += to_str<String_type> ( "\\t"  );
			return true;
	}

	return false;
}

template<class String_type>
String_type add_esc_chars ( const String_type& s, bool raw_utf8, bool esc_nonascii ) {
	typedef typename String_type::const_iterator Iter_type;
	typedef typename String_type::value_type     Char_type;

	String_type result;

	const Iter_type end ( s.end() );

	for ( Iter_type i = s.begin(); i != end; ++i ) {
		const Char_type c ( *i );

		if ( add_esc_char ( c, result ) ) continue;

		if ( raw_utf8 )
			result += c;
		else {
			const wint_t unsigned_c ( ( c >= 0 ) ? c : 256 + c );

			if ( !esc_nonascii && iswprint ( unsigned_c ) )
				result += c;
			else
				result += non_printable_to_string<String_type> ( unsigned_c );
		}
	}

	return result;
}

// this class generates the JSON text,
// it keeps track of the indentation level etc.
//
template<class Value_type, class Ostream_type>
class Generator {
	typedef typename Value_type::Config_type Config_type;
	typedef typename Config_type::String_type String_type;
	typedef typename Config_type::Object_type Object_type;
	typedef typename Config_type::Array_type Array_type;
	typedef typename String_type::value_type Char_type;
	typedef typename Object_type::value_type Obj_member_type;

public:

	Generator ( const Value_type& value, Ostream_type& os, int options, unsigned int precision_of_doubles )
		:   os_ ( os )
		,   indentation_level_ ( 0 )
		,   pretty_ ( ( options & pretty_print ) != 0 || ( options & single_line_arrays ) != 0 )
		,   raw_utf8_ ( ( options & raw_utf8 ) != 0 )
		,   esc_nonascii_ ( ( options & always_escape_nonascii ) != 0 )
		,   single_line_arrays_ ( ( options & single_line_arrays ) != 0 )
		,   ios_saver_ ( os ) {
		if ( precision_of_doubles > 0 )
			precision_of_doubles_ = precision_of_doubles;
		else
			precision_of_doubles_ = ( options & remove_trailing_zeros ) != 0 ? 16 : 17;

		output ( value );
	}

private:

	void output ( const Value_type& value ) {
		switch ( value.type() ) {
			case obj_type:
				output ( value.get_obj() );
				break;
			case array_type:
				output ( value.get_array() );
				break;
			case str_type:
				output ( value.get_str() );
				break;
			case bool_type:
				output ( value.get_bool() );
				break;
			case real_type:
				output ( value.get_real() );
				break;
			case int_type:
				output_int ( value );
				break;
			case null_type:
				os_ << "null";
				break;
			default:
				assert ( false );
		}
	}

	void output ( const Object_type& obj ) {
		output_array_or_obj ( obj, '{', '}' );
	}

	void output ( const Obj_member_type& member ) {
		output ( Config_type::get_name ( member ) );
		space();
		os_ << ':';
		space();
		output ( Config_type::get_value ( member ) );
	}

	void output_int ( const Value_type& value ) {
		if ( value.is_uint64() )
			os_ << value.get_uint64();
		else
			os_ << value.get_int64();
	}

	void output ( const String_type& s ) {
		os_ << '"' << add_esc_chars ( s, raw_utf8_, esc_nonascii_ ) << '"';
	}

	void output ( bool b ) {
		os_ << to_str<String_type> ( b ? "true" : "false" );
	}

	void output ( double d ) {
		os_ << std::setprecision ( precision_of_doubles_ ) << d;
	}

	static bool contains_composite_elements ( const Array_type& arr ) {
		for ( typename Array_type::const_iterator i = arr.begin(); i != arr.end(); ++i ) {
			const Value_type& val = *i;

			if ( val.type() == obj_type ||
			        val.type() == array_type )
				return true;
		}

		return false;
	}

	template<class Iter>
	void output_composite_item ( Iter i, Iter last ) {
		output ( *i );

		if ( ++i != last )
			os_ << ',';
	}

	void output ( const Array_type& arr ) {
		if ( single_line_arrays_ && !contains_composite_elements ( arr )  ) {
			os_ << '[';
			space();

			for ( typename Array_type::const_iterator i = arr.begin(); i != arr.end(); ++i ) {
				output_composite_item ( i, arr.end() );

				space();
			}

			os_ << ']';
		} else
			output_array_or_obj ( arr, '[', ']' );
	}

	template<class T>
	void output_array_or_obj ( const T& t, Char_type start_char, Char_type end_char ) {
		os_ << start_char;
		new_line();

		++indentation_level_;

		for ( typename T::const_iterator i = t.begin(); i != t.end(); ++i ) {
			indent();

			output_composite_item ( i, t.end() );

			new_line();
		}

		--indentation_level_;

		indent();
		os_ << end_char;
	}

	void indent() {
		if ( !pretty_ ) return;

		for ( int i = 0; i < indentation_level_; ++i )
			os_ << "    ";
	}

	void space() {
		if ( pretty_ ) os_ << ' ';
	}

	void new_line() {
		if ( pretty_ ) os_ << '\n';
	}

	Generator& operator= ( const Generator& ); // to prevent "assignment operator could not be generated" warning

	Ostream_type& os_;
	int indentation_level_;
	bool pretty_;
	bool raw_utf8_;
	bool esc_nonascii_;
	bool single_line_arrays_;
	int precision_of_doubles_;
	boost::io::basic_ios_all_saver<Char_type> ios_saver_;  // so that ostream state is reset after control is returned to the caller
};

// writes JSON Value to a stream, e.g.
//
// write_stream( value, os, pretty_print );
//
template<class Value_type, class Ostream_type>
void write_stream ( const Value_type& value, Ostream_type& os, int options = none, unsigned int precision_of_doubles = 0 ) {
	os << std::dec;
	Generator<Value_type, Ostream_type> ( value, os, options, precision_of_doubles );
}

// writes JSON Value to a stream, e.g.
//
// const string json_str = write( value, pretty_print );
//
template<class Value_type>
typename Value_type::String_type write_string ( const Value_type& value, int options = none, unsigned int precision_of_doubles = 0 ) {
	typedef typename Value_type::String_type::value_type Char_type;

	std::basic_ostringstream<Char_type> os;

	write_stream ( value, os, options, precision_of_doubles );

	return os.str();
}
// these classes allows you to read multiple top level contiguous values from a stream,
// the normal stream read functions have a bug that prevent multiple top level values
// from being read unless they are separated by spaces

template<class Istream_type, class Value_type>
class Stream_reader {
public:

	Stream_reader ( Istream_type& is )
		:   iters_ ( is ) {
	}

	bool read_next ( Value_type& value ) {
		return read_range ( iters_.begin_, iters_.end_, value );
	}

private:

	typedef Multi_pass_iters<Istream_type> Mp_iters;

	Mp_iters iters_;
};

template<class Istream_type, class Value_type>
class Stream_reader_thrower {
public:

	Stream_reader_thrower ( Istream_type& is )
		:   iters_ ( is )
		,    posn_begin_ ( iters_.begin_, iters_.end_ )
		,    posn_end_ ( iters_.end_, iters_.end_ ) {
	}

	void read_next ( Value_type& value ) {
		posn_begin_ = read_range_or_throw ( posn_begin_, posn_end_, value );
	}

private:

	typedef Multi_pass_iters<Istream_type> Mp_iters;
	typedef spirit_namespace::position_iterator<typename Mp_iters::Mp_iter> Posn_iter_t;

	Mp_iters iters_;
	Posn_iter_t posn_begin_, posn_end_;
};
// these functions to convert JSON Values to text
// note the precision used outputing doubles defaults to 17,
// unless the remove_trailing_zeros option is given in which case the default is 16

#ifdef JSON_SPIRIT_VALUE_ENABLED
void         write ( const Value&  value, std::ostream&  os, int options = none, unsigned int precision_of_doubles = 0 );
std::string  write ( const Value&  value, int options = none, unsigned int precision_of_doubles = 0 );
#endif

#ifdef JSON_SPIRIT_MVALUE_ENABLED
void         write ( const mValue& value, std::ostream&  os, int options = none, unsigned int precision_of_doubles = 0 );
std::string  write ( const mValue& value, int options = none, unsigned int precision_of_doubles = 0 );
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
void         write ( const wValue&  value, std::wostream& os, int options = none, unsigned int precision_of_doubles = 0 );
std::wstring write ( const wValue&  value, int options = none, unsigned int precision_of_doubles = 0 );
#endif

#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
void         write ( const wmValue& value, std::wostream& os, int options = none, unsigned int precision_of_doubles = 0 );
std::wstring write ( const wmValue& value, int options = none, unsigned int precision_of_doubles = 0 );
#endif

// these "formatted" versions of the "write" functions are the equivalent of the above functions
// with option "pretty_print"

#ifdef JSON_SPIRIT_VALUE_ENABLED
void         write_formatted ( const Value& value, std::ostream&  os, unsigned int precision_of_doubles = 0 );
std::string  write_formatted ( const Value& value, unsigned int precision_of_doubles = 0 );
#endif
#ifdef JSON_SPIRIT_MVALUE_ENABLED
void         write_formatted ( const mValue& value, std::ostream&  os, unsigned int precision_of_doubles = 0 );
std::string  write_formatted ( const mValue& value, unsigned int precision_of_doubles = 0 );
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
void         write_formatted ( const wValue& value, std::wostream& os, unsigned int precision_of_doubles = 0 );
std::wstring write_formatted ( const wValue& value, unsigned int precision_of_doubles = 0 );
#endif
#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
void         write_formatted ( const wmValue& value, std::wostream& os, unsigned int precision_of_doubles = 0 );
std::wstring write_formatted ( const wmValue& value, unsigned int precision_of_doubles = 0 );
#endif
}

#endif
