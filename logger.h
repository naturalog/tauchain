class logger {
public:
	#ifdef DEBUG
	template<typename T> inline logger& operator<< ( const T& t ) {
		std::clog << t;
		return *this;
	}
	template<typename T> inline logger& operator<< ( const obj& t ) {
		std::clog << t.toString();
		return *this;
	}
	template<typename T> inline logger& operator<< ( const pobj& t ) {
		std::clog << ( *t );
		return *this;
	}
	template<typename T> inline logger& operator<< ( const rdf_db& t ) {
		std::clog << t.tostring();
		return *this;
	}
	template<typename T> inline logger& operator<< ( const prdf_db& t ) {
		std::clog << ( *t );
		return *this;
	}
	#else
	template<typename T> inline const logger& operator<< ( const T& t ) const {
		return *this;
	}
	#endif
};
