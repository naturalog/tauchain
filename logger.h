class logger {
public:
	#ifdef DEBUG
	bool pause;
	logger ( bool _pause ) : pause ( _pause ) {
		cin.tie ( &clog );
	}
	void dopause() {
		if clog << "press any key to continue...";
	getchar();
	}
	void before_log() {}
	void after_log() {
		if ( pause ) dopause();
	}
	template<typename T> inline logger& operator<< ( const T& t ) {
		before_log();
		std::clog << t;
		after_log();
		return *this;
	}
	template<typename T> inline logger& operator<< ( const obj& t ) {
		before_log();
		std::clog << t.toString();
		after_log();
		return *this;
	}
	template<typename T> inline logger& operator<< ( const pobj& t ) {
		before_log();
		std::clog << ( *t );
		after_log();
		return *this;
	}
	template<typename T> inline logger& operator<< ( const rdf_db& t ) {
		before_log();
		std::clog << t.tostring();
		after_log();
		return *this;
	}
	template<typename T> inline logger& operator<< ( const prdf_db& t ) {
		before_log();
		std::clog << ( *t );
		after_log();
		return *this;
	}
	#else
	template<typename T> inline logger& operator<< ( T t ) {
		return *this;
	}
	#endif
};
