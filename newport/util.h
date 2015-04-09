
template<typename T>
inline bool is ( const T& s, const std::vector<T>& v, std::string exception = std::string() ) {
	bool rc = std::find ( v.begin(), v.end(), s ) != v.end();
	if ( exception.size() && !rc ) throw exception;
	return rc;
}

template<typename T> inline bool is ( const std::shared_ptr<T>& s, const std::vector<T>& v, std::string exception = std::string() ) {
	return is<T> ( *s, v, exception );
}


inline string lower ( const string& s_ ) {
	string s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::tolower );
	return s;
}

inline bool endsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( x.size() - y.size(), y.size() ) == y;
}
inline bool startsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( 0, y.size() ) == y;
}

template<typename charT>
inline vector<string> split ( const string& s, charT c ) {
	vector<string> v;
	for ( string::size_type i = 0,  j = s.find ( c ); j != string::npos; ) {
		v.push_back ( s.substr ( i, j - i ) );
		j = s.find ( c, i = ++j );
		if ( j == string::npos ) v.push_back ( s.substr ( i, s.length() ) );
	}
	return v;
}

template<typename C, typename K> bool has ( const C& c, const K& k ) {
	return /*std::find(c.begin(), c.end(), k)*/c.find ( k ) != c.end();
}
template<typename C, typename K> bool has ( std::shared_ptr<C> c, const K& k ) {
	return c && has<C, K> ( *c, k );
}
template<typename C, typename K> bool has ( std::shared_ptr<C> c, std::shared_ptr<K> k ) {
	return k && has<C, K> ( c, *k );
}
