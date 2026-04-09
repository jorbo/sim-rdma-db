#ifndef CPP_EXT_HPP
#define CPP_EXT_HPP


#include "operations.h"
#include <string>


bool operator==(const Response& lhs, const Response& rhs);
bool operator!=(const Response& lhs, const Response& rhs);
std::string to_str(const Response& resp);


#endif
