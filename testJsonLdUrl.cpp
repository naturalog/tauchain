#include <iostream>
#include <iterator>
#include <string>
#include <boost/regex.hpp>

//#include <boost/regex/icu.hpp>
//#include <regex>

#include "JsonLdUrl.h"

//typedef std::wbasic_regex<std::wstring> Pattern;
//const wchar_t xxx[6] = L"[a-\xEFFFF]";
//std::wregex pattern(xxx);

int main() 
{
	JsonLdUrl *r = JsonLdUrl::parse("http://u:p@caa.bb/cc#dd");
	r->debugPrint();
}

//	std::string s = "Boost Libraries";
//	regex expr {"\\w+\\s\\w+"};
//	std::cout << std::boolalpha << regex_match ( s, expr ) << '\n';
