//
// Created by kook on 27.2.16.
//

#include <string>
#include <set>
#include <iostream>


int result_limit = 123;
std::set<std::string> silence;
bool in_silent_part = false;

std::ostream& dout = std::cout;
std::ostream& derr = std::cerr;
std::istream& din = std::cin;

