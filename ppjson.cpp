#include "json_spirit.h"
#include <iostream>

int main() {
	json_spirit::Value v;
	json_spirit::read_stream ( std::cin, v );
	std::cout << json_spirit::write_string ( v, /*json_spirit::pretty_print | json_spirit::single_line_arrays*/ 0 ) << std::endl;
	return 0;
}
