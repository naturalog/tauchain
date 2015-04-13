// based on https://github.com/jaredhoberock/hindley_milner
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <set>
#include <utility>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/variant/static_visitor.hpp>

class integer_literal {
public:
	inline integer_literal ( const int v ) : m_value ( v ) { } 
	int value ( void ) const { return m_value; } 
private:
	int m_value;
};

inline std::ostream &operator<< ( std::ostream &os, const integer_literal &il ) { return os << il.value(); } // end operator<<() 

class identifier {
public:
	inline identifier ( const std::string &name ) : m_name ( name ) { } 
	inline const std::string &name ( void ) const { return m_name; } 
private:
	std::string m_name;
};

inline std::ostream &operator<< ( std::ostream &os, const identifier &i ) { return os << i.name(); }

class apply;
class lambda;
class let;
class letrec;

typedef boost::variant < integer_literal, identifier, boost::recursive_wrapper<apply>, boost::recursive_wrapper<lambda>, boost::recursive_wrapper<let>, boost::recursive_wrapper<letrec> > node;

class apply {
public:
	inline apply ( node &&fn, node &&arg ) : m_fn ( std::move ( fn ) ), m_arg ( std::move ( arg ) ) { } 
	inline apply ( const node &fn, const node &arg ) : m_fn ( fn ), m_arg ( arg ) { } 
	inline const node &function ( void ) const { return m_fn; } 
	inline const node &argument ( void ) const { return m_arg; } 
private:
	node m_fn, m_arg;
};

inline std::ostream &operator<< ( std::ostream &os, const apply &a ) { return os << "(" << a.function() << " " << a.argument() << ")"; }

class lambda {
public:
	inline lambda ( const std::string &param, node &&body ) : m_param ( param ), m_body ( std::move ( body ) ) { } 
	inline lambda ( const std::string &param, const node &body ) : m_param ( param ), m_body ( body ) { } 
	inline const std::string &parameter ( void ) const { return m_param; } 
	inline const node &body ( void ) const { return m_body; } 
private:
	std::string m_param;
	node m_body;
};

inline std::ostream &operator<< ( std::ostream &os, const lambda &l ) { return os << "(fn " << l.parameter() << " => " << l.body() << ")"; }

class let {
public:
	inline let ( const std::string &name, node &&def, node &&body ) : m_name ( name ),
		  m_definition ( std::move ( def ) ), m_body ( std::move ( body ) ) { } 
	inline let ( const std::string &name, const node &def, const node &body ) : m_name ( name ),
		  m_definition ( def ), m_body ( body ) { }

	inline const std::string &name() const { return m_name; } 
	inline const node &definition() const { return m_definition; } 
	inline const node &body() const { return m_body; } 
private:
	std::string m_name;
	node m_definition, m_body;
};

inline std::ostream &operator<< ( std::ostream &os, const let &l ) { return os << "(let " << l.name() << " = " << l.definition() << " in " << l.body() << ")"; }

class letrec {
public:
	inline letrec ( const std::string &name, node &&def, node &&body ) : m_name ( name ), m_definition ( std::move ( def ) ), m_body ( std::move ( body ) ) { } 
	inline letrec ( const std::string &name, const node &def, const node &body ) : m_name ( name ), m_definition ( def ), m_body ( body ) { } 
	inline const std::string &name() const { return m_name; } 
	inline const node &definition() const { return m_definition; } 
	inline const node &body() const { return m_body; } 
private:
	std::string m_name;
	node m_definition, m_body;
};

inline std::ostream &operator<< ( std::ostream &os, const letrec &l ) { return os << "(letrec " << l.name() << " = " << l.definition() << " in " << l.body() << ")"; }

class type_variable;
class type_operator;

typedef boost::variant < type_variable, boost::recursive_wrapper<type_operator> > type;

class type_variable {
public:
	inline type_variable() : m_id() { } 
	inline type_variable ( const std::size_t i ) : m_id ( i ) { } 
	inline std::size_t id() const { return m_id; } 
	inline bool operator== ( const type_variable &other ) const { return id() == other.id(); } 
	inline bool operator!= ( const type_variable &other ) const { return ! ( *this == other ); } 
	inline bool operator< ( const type_variable &other ) const { return id() < other.id(); } 
	inline operator std::size_t ( void ) const { return id(); } 
private:
	std::size_t m_id;
};

class type_operator : private std::vector<type> {
public:
	typedef std::size_t kind_type;
private:
	typedef std::vector<type> super_t;
	std::vector<type> m_types;
	kind_type m_kind;
public:
	using super_t::begin;
	using super_t::end;
	using super_t::size;
	using super_t::operator[];

	inline type_operator ( const type_operator &other ) : super_t ( other ), m_types ( other.m_types ), m_kind ( other.m_kind ) { } 
	inline type_operator ( const kind_type &kind ) : m_kind ( kind ) { } 
	template<typename Iterator> type_operator ( const kind_type &kind, Iterator first, Iterator last ) : super_t ( first, last ), m_kind ( kind ) { } 
	template<typename Range> inline type_operator ( const kind_type &kind, const Range &rng ) : super_t ( rng.begin(), rng.end() ), m_kind ( kind ) { } 
	inline type_operator ( const kind_type &kind, std::initializer_list<type> &&types ) : super_t ( types ), m_kind ( kind ) { } 
	inline type_operator ( type_operator &&other ) : super_t ( std::move ( other ) ), m_kind ( std::move ( other.m_kind ) ) { } 
	inline type_operator &operator= ( const type_operator &other ) {
		super_t::operator= ( other );
		m_types = other.m_types;
		m_kind  = other.m_kind;
		return *this;
	} 
	inline const kind_type &kind ( void ) const { return m_kind; } 
	inline bool compare_kind ( const type_operator &other ) const { return kind() == other.kind() && size() == other.size(); } 
	inline bool operator== ( const type_operator &other ) const { return compare_kind ( other ) & std::equal ( begin(), end(), other.begin() ); } 
};

typedef std::pair<type, type> constraint;

struct type_mismatch : std::runtime_error {
	inline type_mismatch ( const type &xx, const type &yy ) : std::runtime_error ( "type mismatch" ), x ( xx ), y ( yy ) { } 
	inline virtual ~type_mismatch ( void ) throw() { } 
	type x;
	type y;
};

struct recursive_unification : std::runtime_error {
	inline recursive_unification ( const type &xx, const type &yy ) : std::runtime_error ( "recursive unification" ), x ( xx ), y ( yy ) { } 
	inline virtual ~recursive_unification ( void ) throw() { } 
	type x, y;
};

namespace detail {

inline void replace ( type &x, const type_variable &replace_me, const type &replacement ) {
	if ( x.which() ) {
		auto &op = boost::get<type_operator> ( x );
		auto f = std::bind ( replace, std::placeholders::_1, replace_me, replacement );
		std::for_each ( op.begin(), op.end(), f );
	} 
	else {
		auto &var = boost::get<type_variable> ( x );
		if ( var == replace_me ) x = replacement; 
	} 
} 

inline bool occurs ( const type &haystack, const type_variable &needle ) {
	bool result = false;
	if ( haystack.which() ) {
		auto &op = boost::get<type_operator> ( haystack );
		auto f = std::bind ( occurs, std::placeholders::_1, needle );
		result = std::any_of ( op.begin(), op.end(), f );
	} 
	else {
		auto &var = boost::get<type_variable> ( haystack );
		result = ( var == needle );
	} 

	return result;
} 

struct equals_variable : boost::static_visitor<bool> {
	inline equals_variable ( const type_variable &xx ) : m_x ( xx ) { } 
	inline bool operator() ( const type_variable &y ) { return m_x == y; } 
	inline bool operator() ( const type_operator &y ) { return false; } 
	const type_variable &m_x;
};

struct replacer : boost::static_visitor<> {
	inline replacer ( const type_variable &replace_me ) : m_replace_me ( replace_me ) { } 
	inline void operator() ( type_variable &var, const type_variable &replacement ) { if ( var == m_replace_me ) var = replacement; } 
	template<typename T> inline void operator() ( type_operator &op, const T &replacement ) {
		auto v = boost::apply_visitor ( *this );
		auto f = std::bind ( v, std::placeholders::_2, replacement );
		std::for_each ( op.begin(), op.end(), f );
	} 
	const type_variable &m_replace_me;
};

class unifier : public boost::static_visitor<> {
	inline void eliminate ( const type_variable &x, const type &y ) {
		// replace all occurrances of x with y in the stack and the substitution
		for ( auto i = m_stack.begin(); i != m_stack.end(); ++i ) {
			replace ( i->first, x, y );
			replace ( i->second, x, y );
		} 
		for ( auto i = m_substitution.begin(); i != m_substitution.end(); ++i ) replace ( i->second, x, y );
		m_substitution[x] = y;// add x = y to the substitution
	} 
	std::vector<constraint>       m_stack;
	std::map<type_variable, type> &m_substitution;
public:
	// apply_visitor requires that these functions be public
	inline void operator() ( const type_variable &x, const type_variable &y ) { if ( x != y ) eliminate ( x, y ); } 
	inline void operator() ( const type_variable &x, const type_operator &y ) {
		if ( occurs ( y, x ) ) throw recursive_unification ( x, y ); 
		eliminate ( x, y );
	} 
	inline void operator() ( const type_operator &x, const type_variable &y ) {
		if ( occurs ( x, y ) ) throw recursive_unification ( y, x ); 
		eliminate ( y, x );
	} 
	inline void operator() ( const type_operator &x, const type_operator &y ) {
		if ( !x.compare_kind ( y ) ) throw type_mismatch ( x, y );
		// push (xi,yi) onto the stack
		for ( auto xi = x.begin(), yi = y.begin(); xi != x.end(); ++xi, ++yi ) m_stack.push_back ( std::make_pair ( *xi, *yi ) );
	} 
	template<typename Iterator>
	inline unifier ( Iterator first_constraint, Iterator last_constraint, std::map<type_variable, type> &substitution )
		: m_stack ( first_constraint, last_constraint ), m_substitution ( substitution ) {
		// add the current substitution to the stack
		// XXX this step might be unnecessary
		m_stack.insert ( m_stack.end(), m_substitution.begin(), m_substitution.end() );
		m_substitution.clear();
	}

	inline void operator() ( void ) {
		while ( !m_stack.empty() ) {
			type x = std::move ( m_stack.back().first ), y = std::move ( m_stack.back().second );
			m_stack.pop_back(); 
			boost::apply_visitor ( *this, x, y );
		}
	}
};

}

template<typename Iterator> void unify ( Iterator first_constraint, Iterator last_constraint, std::map<type_variable, type> &substitution ) {
	detail::unifier ( first_constraint, last_constraint, substitution )();
} 
template<typename Range> void unify ( const Range &rng, std::map<type_variable, type> &substitution ) {
	return unify ( rng.begin(), rng.end(), substitution );
} 
void unify ( const type &x, const type &y, std::map<type_variable, type> &substitution ) {
	auto c = constraint ( x, y );
	return unify ( &c, &c + 1, substitution );
}
template<typename Range> std::map<type_variable, type> unify ( const Range &rng ) {
	std::map<type_variable, type> solutions;
	unify ( rng, solutions );
	return std::move ( solutions );
}
}

using gtype;
using gtype_variable;
using gtype_operator;

std::ostream &operator<< ( std::ostream &os, const std::set<type_variable> &x ) {
	for ( auto i = x.begin(); i != x.end(); ++i ) os << *i << " "; 
	return os;
}

std::ostream &operator<< ( std::ostream &os, const std::map<type_variable, type_variable> &x ) {
	os << "{";
	for ( auto i = x.begin(); i != x.end(); ++i ) os << i->first << " : " << i->second << ", "; os << "}"; 
	return os;
}

namespace types {

static const int integer  = 0;
static const int boolean  = 1;
static const int function = 2;
static const int pair     = 3;

}

inline type make_function ( const type &arg, const type &result ) { return type_operator ( types::function, {arg, result} ); } 
inline type integer ( void ) { return type_operator ( types::integer ); } 
inline type boolean ( void ) { return type_operator ( types::boolean ); } 
inline type pair ( const type &first, const type &second ) { return type_operator ( types::pair, {first, second} ); } 
inline type definitive ( const std::map<type_variable, type> &substitution, const type_variable &x ) {
	type result = x;
	// iteratively follow type_variables in the substitution until we can't go any further
	type_variable *ptr = 0;
	while ( ( ptr = boost::get<type_variable> ( &result ) ) && substitution.count ( *ptr ) )
		result = substitution.find ( *ptr )->second; 
	return result;
}

class environment : public std::map<std::string, type> {
public:
	inline environment() : m_next_id ( 0 ) { } 
	inline std::size_t unique_id() { return m_next_id++; } 
private:
	std::size_t m_next_id;
};

struct fresh_maker : boost::static_visitor<type> {
	inline fresh_maker ( environment &env, const std::set<type_variable> &non_generic, const std::map<type_variable, type> &substitution ) : 
		m_env ( env ), m_non_generic ( non_generic ), m_substitution ( substitution ) { }

	inline result_type operator() ( const type_variable &var ) {
		if ( is_generic ( var ) ) {
			if ( !m_mappings.count ( var ) ) m_mappings[var] = type_variable ( m_env.unique_id() );
			return m_mappings[var];
		} 
		return var;
	} 

	inline result_type operator() ( const type_operator &op ) {
		std::vector<type> types ( op.size() );
		// make sure to pass a reference to this to maintain our state
		std::transform ( op.begin(), op.end(), types.begin(), std::ref ( *this ) );
		return type_operator ( op.kind(), types );
	} 

	inline result_type operator() ( const type &x ) {
		result_type result;
		// XXX hard-coded 0 here sucks
		if ( x.which() == 0 ) {
			auto definitive_type = definitive ( m_substitution, boost::get<type_variable> ( x ) );
			result = boost::apply_visitor ( *this, definitive_type );
		} 
		else result = boost::apply_visitor ( *this, x ); // end else
		return result;
	}
private:
	inline bool is_generic ( const type_variable &var ) const {
		bool occurs = false;
		for ( auto i = m_non_generic.begin(); i != m_non_generic.end(); ++i ) {
			occurs = gdetail::occurs ( definitive ( m_substitution, *i ), var );
			if ( occurs ) break;
		}
		return !occurs;
	}

	environment                           &m_env;
	const std::set<type_variable>         &m_non_generic;
	const std::map<type_variable, type>    &m_substitution;
	std::map<type_variable, type_variable> m_mappings;
}; // end fresh_maker

struct inferencer : boost::static_visitor<type> {
	inline inferencer ( const environment &env ) : m_environment ( env ) { } 
	inline result_type operator() ( const ginteger_literal ) { return integer(); }
	inline result_type operator() ( const gidentifier &id ) {
		if ( !m_environment.count ( id.name() ) ) {
			auto what = std::string ( "Undefined symbol " ) + id.name();
			throw std::runtime_error ( what );
		} 
		// create a fresh type
		auto freshen_me = m_environment[id.name()];
		auto v = fresh_maker ( m_environment, m_non_generic_variables, m_substitution );
		return v ( freshen_me );
	}
	inline result_type operator() ( const gapply &app ) {
		auto fun_type = boost::apply_visitor ( *this, app.function() );
		auto arg_type = boost::apply_visitor ( *this, app.argument() );
		auto x = type_variable ( m_environment.unique_id() );
		auto lhs = make_function ( arg_type, x );
		gunify ( lhs, fun_type, m_substitution );
		return definitive ( m_substitution, x );
	}
	inline result_type operator() ( const glambda &lambda ) {
		auto arg_type = type_variable ( m_environment.unique_id() );
		auto s = scoped_non_generic_variable ( this, lambda.parameter(), arg_type );
		auto body_type = boost::apply_visitor ( *this, lambda.body() );
		auto x = type_variable ( m_environment.unique_id() );
		gunify ( x, make_function ( arg_type, body_type ), m_substitution );
		return definitive ( m_substitution, x );
	} 
	inline result_type operator() ( const glet &let ) {
		auto defn_type = boost::apply_visitor ( *this, let.definition() );
		// introduce a scope with a generic variable
		auto s = scoped_generic ( this, let.name(), defn_type );
		auto result = boost::apply_visitor ( *this, let.body() );
		return result;
	} // end operator()()

	inline result_type operator() ( const gletrec &letrec ) {
		auto new_type = type_variable ( m_environment.unique_id() );
		auto s = scoped_non_generic_variable ( this, letrec.name(), new_type );
		auto definition_type = boost::apply_visitor ( *this, letrec.definition() );
		gunify ( new_type, definition_type, m_substitution );
		auto result = boost::apply_visitor ( *this, letrec.body() );
		return result;
	}

	struct scoped_generic {
		inline scoped_generic ( inferencer *inf, const std::string &name, const type &t ) : m_environment ( inf->m_environment ) {
			auto iter = m_environment.find ( name ); 
			if ( iter != m_environment.end() ) {
				m_restore = std::make_tuple ( true, iter, iter->second );
				iter->second = t;
			}
			else m_restore = std::make_tuple ( false, m_environment.insert ( std::make_pair ( name, t ) ).first, type() );
		} 

		inline ~scoped_generic() {
			using namespace std;
			if ( get<0> ( m_restore ) ) get<1> ( m_restore )->second = get<2> ( m_restore );
			else m_environment.erase ( get<1> ( m_restore ) );
		}
		environment &m_environment;
		std::tuple<bool, environment::iterator, type> m_restore;
	};

	struct scoped_non_generic_variable : scoped_generic {
		inline scoped_non_generic_variable ( inferencer *inf, const std::string &name, const type_variable &var )
			: scoped_generic ( inf, name, var ), m_non_generic ( inf->m_non_generic_variables ), m_erase_me ( m_non_generic.insert ( var ) ) { } 
		inline ~scoped_non_generic_variable() { if ( m_erase_me.second ) m_non_generic.erase ( m_erase_me.first ); // end if } // end ~scoped_non_generic_variable()
		std::set<type_variable>                           &m_non_generic;
		std::pair<std::set<type_variable>::iterator, bool> m_erase_me;
	};

	environment                         m_environment;
	std::set<type_variable>             m_non_generic_variables;
	std::map<type_variable, type>        m_substitution;
};

type infer_type ( const gnode &node, const environment &env ) {
	auto v = inferencer ( env );
	auto old = std::clog.rdbuf ( 0 );
	auto result = boost::apply_visitor ( v, node );
	std::clog.rdbuf ( old );
	return result;
}

namespace types {
static const int integer  = 0;
static const int boolean  = 1;
static const int function = 2;
static const int pair     = 3;
}

class pretty_printer
	: public boost::static_visitor<pretty_printer&> {
public:
	inline pretty_printer ( std::ostream &os )
		: m_os ( os ),
		  m_next_name ( 'a' ) {
	}

	inline pretty_printer &operator() ( const gtype_variable &x ) {
		if ( !m_names.count ( x ) ) {
			std::ostringstream os;
			os << m_next_name++;
			m_names[x] = os.str();
		} // end if

		m_os << m_names[x];
		return *this;
	}

	inline pretty_printer &operator() ( const gtype_operator &x ) {
		switch ( x.kind() ) {
			case types::integer: {
					m_os << "int";
					break;
				} // end case
			case types::boolean: {
					m_os << "bool";
					break;
				} // end case
			case types::function: {
					m_os << "(";
					*this << x[0];
					m_os << " -> ";
					*this << x[1];
					m_os << ")";
					break;
				} // end case
			case types::pair: {
					m_os << "(";
					*this << x[0];
					m_os << " * ";
					*this << x[1];
					m_os << ")";
					break;
				} // end case
			default: {
				} // end default
		} // end switch

		return *this;
	}

	inline pretty_printer &operator<< ( const gtype &x ) {
		return boost::apply_visitor ( *this, x );
	}

	inline pretty_printer &operator<< ( std::ostream & ( *fp ) ( std::ostream & ) ) {
		fp ( m_os );
		return *this;
	}

	template<typename T>
	inline pretty_printer &operator<< ( const T &x ) {
		m_os << x;
		return *this;
	}

private:
	std::ostream &m_os;

	std::map<gtype_variable, std::string> m_names;
	char m_next_name;
};

namespace unification {

inline std::ostream &operator<< ( std::ostream &os, const type_variable &x ) {
	pretty_printer pp ( os );
	pp << type ( x );
	return os;
}

inline std::ostream &operator<< ( std::ostream &os, const type_operator &x ) {
	pretty_printer pp ( os );
	pp << type ( x );
	return os;
}

}

struct try_to_infer {
	inline try_to_infer ( const genvironment &e )
		: env ( e ) {
	}

	inline void operator() ( const gnode &n ) const {
		try {
			auto result = ginfer_type ( n, env );

			std::cout << n << " : ";
			pretty_printer pp ( std::cout );
			pp << result << std::endl;
		} // end try
		catch ( const grecursive_unification &e ) {
			std::cerr << n << " : ";
			pretty_printer pp ( std::cerr );
			pp << e.what() << ": " << e.x << " in " << e.y << std::endl;
		} // end catch
		catch ( const gtype_mismatch &e ) {
			std::cerr << n << " : ";
			pretty_printer pp ( std::cerr );
			pp << e.what() << ": " << e.x << " != " << e.y << std::endl;
		} // end catch
		catch ( const std::runtime_error &e ) {
			std::cerr << n << " : " << e.what() << std::endl;
		} // end catch
	} // end operator()

	const genvironment &env;
};

int main() {
	environment env;
	std::vector<node> examples;

	auto var1 = type_variable ( env.unique_id() );
	auto var2 = type_variable ( env.unique_id() );
	auto var3 = type_variable ( env.unique_id() );

	env["pair"] = make_function ( var1, gmake_function ( var2, pair ( var1, var2 ) ) );
	env["true"] = boolean();
	env["cond"] = make_function (
	                  boolean(),
	                  make_function (
	                      var3, make_function (
	                          var3, var3
	                      )
	                  )
	              );
	env["zero"] = make_function ( integer(), boolean() );
	env["pred"] = make_function ( integer(), integer() );
	env["times"] = make_function (
	                   integer(), make_function (
	                       integer(), integer()
	                   )
	               );

	auto pair = apply ( apply ( identifier ( "pair" ), apply ( identifier ( "f" ), integer_literal ( 4 ) ) ), apply ( identifier ( "f" ), identifier ( "true" ) ) );

	// factorial
	{
		auto example =
		    letrec ( "factorial",
		             lambda ( "n",
		                      apply (
		                          apply (
		                              apply ( identifier ( "cond" ),
		                                      apply ( identifier ( "zero" ), identifier ( "n" ) )
		                                    ),
		                              integer_literal ( 1 )
		                          ),
		                          apply (
		                              apply ( identifier ( "times" ), identifier ( "n" ) ),
		                              apply ( identifier ( "factorial" ),
		                                      apply ( identifier ( "pred" ), identifier ( "n" ) )
		                                    )
		                          )
		                      )
		                    ),
		             apply ( identifier ( "factorial" ), integer_literal ( 5 ) )
		           );
		examples.push_back ( example );
	}

	// fn x => (pair(x(3) (x(true)))
	{
		auto example = lambda ( "x",
		                        apply (
		                            apply ( identifier ( "pair" ),
		                                    apply ( identifier ( "x" ), integer_literal ( 3 ) ) ),
		                            apply ( identifier ( "x" ), identifier ( "true" ) ) ) );
		examples.push_back ( example );
	}

	// pair(f(3), f(true))
	{
		auto example =
		    apply (
		        apply ( identifier ( "pair" ), apply ( identifier ( "f" ), integer_literal ( 4 ) ) ),
		        apply ( identifier ( "f" ), identifier ( "true" ) )
		    );
		examples.push_back ( example );
	}

	// let f = (fn x => x) in ((pair (f 4)) (f true))
	{
		auto example = let ( "f", lambda ( "x", identifier ( "x" ) ), pair );
		examples.push_back ( example );
	}

	// fn f => f f (fail)
	{
		auto example = lambda ( "f", apply ( identifier ( "f" ), identifier ( "f" ) ) );
		examples.push_back ( example );
	}

	// let g = fn f => 5 in g g
	{
		auto example = let ( "g",
		                     lambda ( "f", integer_literal ( 5 ) ),
		                     apply ( identifier ( "g" ), identifier ( "g" ) ) );
		examples.push_back ( example );
	}

	// example that demonstrates generic and non-generic variables
	// fn g => let f = fn x => g in pair (f 3, f true)
	{
		auto example =
		    lambda ( "g",
		             let ( "f",
		                   lambda ( "x", identifier ( "g" ) ),
		                   apply (
		                       apply ( identifier ( "pair" ),
		                               apply ( identifier ( "f" ), integer_literal ( 3 ) )
		                             ),
		                       apply ( identifier ( "f" ), identifier ( "true" ) )
		                   )
		                 )
		           );
		examples.push_back ( example );
	}

	// function composition
	// fn f (fn g (fn arg (f g arg)))
	{
		auto example = lambda ( "f", lambda ( "g", lambda ( "arg", apply ( identifier ( "g" ), apply ( identifier ( "f" ), identifier ( "arg" ) ) ) ) ) );
		examples.push_back ( example );
	}

	// fn f => f 5
	{
		auto example = lambda ( "f", apply ( identifier ( "f" ), integer_literal ( 5 ) ) );
		examples.push_back ( example );
	}

	// f = fn x => 1
	// g = fn y => y 1
	// (g f)
	{
		auto return_one = lambda ( "x", integer_literal ( 1 ) );
		auto apply_one  = lambda ( "y", apply ( identifier ( "y" ), integer_literal ( 1 ) ) );
		auto example = apply ( apply_one, return_one );
		examples.push_back ( example );
	}

	auto f = try_to_infer ( env );
	std::for_each ( examples.begin(), examples.end(), f );

	return 0;
}

